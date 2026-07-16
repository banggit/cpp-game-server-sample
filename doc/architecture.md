# Architecture

이 문서는 코드의 모양보다 **왜 이렇게 짰는가** 에 초점을 둔다. 빌드/실행 방법은 [README.md](../README.md) 참조.

## 개요

작은 규모의 C++ 게임 서버 샘플. 풀 MMORPG 서버가 아니라, 실무 게임 서버에서 반복적으로 등장하는 **구조적 패턴** 을 작은 규모로 시연하기 위해 만들었다.

핵심은 네 가지.

1. 워커 기반 잡 디스패치
2. 네트워크 / 게임 로직 / DB I/O 스레드 분리
3. 메모리 우선 처리 + DB 비동기 기록
4. 게임 객체 단위의 주기 갱신

## 스레드 모델

```
┌─────────────────────────────────────────────────────────────┐
│                       Main Thread                            │
│                  (asio io_context.run)                       │
│                                                              │
│   - accept loop                                              │
│   - per-session async_receive / async_send                   │
│   - signal handling (SIGINT / SIGTERM)                       │
└─────────────────────────────────────────────────────────────┘
                              │
                              │  Job (PACKET_PROCESS)
                              ▼
┌─────────────────────────────────────────────────────────────┐
│                    GameWorker Thread                         │
│                                                              │
│   - packet dispatch (PacketHandler)                          │
│   - game logic (login, user lifecycle, ...)                  │
│   - periodic update (every 5s) → User::OnUpdate              │
└─────────────────────────────────────────────────────────────┘
                              │
                              │  Job (DB_LOG_LOGIN)
                              ▼  (fire-and-forget)
┌─────────────────────────────────────────────────────────────┐
│                     DbWorker Thread                          │
│                                                              │
│   - simulated DB I/O (sleep 50ms)                            │
│   - no callback to GameWorker                                │
└─────────────────────────────────────────────────────────────┘
```

메인 스레드는 asio io_context.run() 으로 네트워크 reactor 역할만 한다. 실무 코드의 별도 Reactor 워커와 같은 역할을 asio 가 자체적으로 제공해주기 때문에, 굳이 한 겹 더 감싸지 않았다.

게임 로직은 단일 GameWorker 스레드에서만 실행된다. 게임 서버에서 게임 로직 스레드를 단일로 두는 이유는 동시성 제어 비용이 멀티 스레드 락 경쟁 비용보다 싸기 때문 — 모든 상태 변경이 한 스레드에서만 일어나면 락이 거의 필요 없다.

## 워커 패턴

`Worker` 베이스 클래스가 자체 스레드와 Job 큐를 소유한다. 파생 클래스는 `ProcessJob()` 만 구현하면 된다.

```cpp
class Worker : public std::enable_shared_from_this<Worker>
{
protected:
    virtual void OnCreate()  {}
    virtual void OnUpdate()  {}
    virtual void OnDestroy() {}
    virtual void OnRun();                       // 큐에서 Pop 후 ProcessJob 호출
    virtual void ProcessJob(const Job&) = 0;

private:
    void ThreadMain()
    {
        OnCreate();
        while (!m_shutdown_requested || HasJob())
        {
            OnUpdate();
            OnRun();
            if (!HasJob()) sleep(1ms);
        }
        OnDestroy();
    }

    std::deque<Job>     m_queue;
    std::mutex          m_mutex;
    std::thread         m_thread;
    std::atomic<bool>   m_shutdown_requested;
};
```

`WorkerManager` 가 워커들을 이름(string)으로 등록/조회/일괄 종료 관리한다. 등록 시점에 워커 스레드가 시작되고, `Destroy()` 시점에 모든 워커에 종료 신호를 보낸 후 join 한다.

### 왜 워커 패턴을 도입했는가

처음에는 LogicWorker 한 개에 std::thread 만 직접 들고 있는 단순한 구조였다. 다만 다음 두 가지를 시연하면서 워커 베이스 추상화가 정당화된다.

- GameWorker (게임 로직 전용)
- DbWorker (DB I/O 전용)

두 워커 모두 "자체 스레드 + Job 큐 + 메시지 처리" 패턴이 동일하기 때문에 베이스 클래스로 묶는 게 자연스럽다. 추후 RedisWorker, MatchingWorker 등이 추가될 때도 같은 패턴으로 확장된다.

### Heartbeat 는 왜 별도 워커가 아닌가

처음에는 HeartbeatTimer 를 별도 스레드로 두었지만, 실제 게임 서버 코드에서는 **각 User 객체가 자기 세션의 timeout 을 검사**하는 패턴이 흔하다. GameWorker 가 매 5초마다 `UserManager::ForEachUser()` 로 모든 User 의 `OnUpdate()` 를 호출하고, User 가 자기 세션의 마지막 활동 시간을 검사한다.

이 패턴의 장점.

- User 단위 책임이 명확하다 (timeout 외 버프 만료, 자동 회복 등도 같은 자리에서 추가 가능)
- 게임 로직 스레드 안에서 실행되므로 SessionManager 락 경쟁이 없다
- 로그인 안 한 세션은 timeout 검사 대상이 아니다 (User 가 없으므로) — 게임서버 입장에서 자연스러운 동작
- 별도 워커 1개를 차지할 만큼 무거운 작업이 아니다 (5초마다 짧은 순회)

## 패킷 처리 파이프라인

```
[Network Thread]
  socket recv
    ↓
  Session::OnReceiveComplete
    ↓
  PacketBuffer::TryReadPacket           ← partial 패킷 조립
    ↓
  GameWorker::Push(PACKET_PROCESS Job)  ← 큐에 적재
    ↓
[GameWorker Thread]
  GameWorker::ProcessJob
    ↓
  PacketHandler::Dispatch               ← opcode 분기
    ↓
  HandleLogin / HandleEcho / HandleHeartbeat
```

네트워크 콜백 안에서 **로직을 직접 실행하지 않는다**. 콜백은 패킷을 조립해 Job 큐에 던지는 역할만 한다. 이렇게 분리하면.

- I/O 콜백이 절대 블로킹되지 않는다
- 게임 상태 변경이 단일 스레드에서만 일어나므로 락 비용이 없다
- 부하가 몰릴 때 Job 큐가 버퍼 역할을 해준다. 실제로 처리 한계를 초과하는 부하에서도 큐가 무한히 발산하지 않고 일정 깊이에서 준평형을 이루는 것을 측정으로 확인했다 (아래 "부하 측정" 참고)

## LOGIN 흐름 — 메모리 우선 + DB fire-and-forget

게임 서버에서 자주 잘못 짜는 패턴이 "DB 응답을 기다린 후 메모리에 반영" 이다. 이 흐름은 응답 시간이 곧 DB latency 가 되어 버린다. MMO 환경에서 사용자가 매번 DB 라운드트립을 기다리는 건 받아들일 수 없다.

이 샘플의 LOGIN 흐름.

```
LOGIN_REQ (account_id)
    ↓
[GameWorker]
    1. UserManager::CreateUser              ← 메모리에 즉시 생성
    2. Session::BindUser                    ← 세션에 즉시 바인딩
    3. SendPacket(LOGIN_ACK)                ← 즉시 응답 (수 마이크로초)
    4. DbWorker::Push(DB_LOG_LOGIN)         ← 비동기 DB 기록 (fire-and-forget)
    ↓
[DbWorker - 별도 스레드]
    sleep(50ms)                             ← DB I/O 시뮬
    log("login record written")             ← GameWorker 로 콜백 안 함
```

핵심 결정.

- **응답 시간 = 메모리 처리 시간**. DB latency 가 사용자 응답에 영향을 주지 않는다.
- **DB 는 진실의 근원이 아니라 백업**. 게임 상태의 진실은 메모리에 있다.
- **fire-and-forget**. DB 실패 시 로그만 남긴다 (실제 코드라면 retry 큐로 보낼 자리).

DB 응답을 기다려야 하는 케이스 (예: 캐릭터 인벤토리 조회) 도 게임 서버에서 흔히 있지만, 그것조차 보통 **선읽기 + 메모리 캐시** 로 해결한다.

## ID 체계

게임 서버에서 흔히 두 가지 ID 를 분리한다.

- `account_id`: 계정 식별 (인증 시스템 / DB account 테이블 PK)
- `user_id`: 게임 내 캐릭터 식별 (DB user 테이블 PK)

한 계정으로 여러 캐릭터를 만드는 MMORPG 의 표준 패턴이다. 샘플에서는 1계정-1캐릭터로 단순화하되, ID 분리는 유지한다.

```
LOGIN_REQ payload : account_id (uint64)
LOGIN_ACK payload : result (uint8) + user_id (uint64)
```

`UserManager::ResolveUserId()` 가 account_id → user_id 변환을 담당한다 (실제로는 DB 조회, 샘플에서는 mock 매핑 `account_id - 1000 + 1`).

## 세션 lifecycle

```
[accept]
    ↓
SessionManager::CreateSession             ← shared_ptr 보관
    ↓
Session::Start (recv loop 시작)
    ↓
[network 활동 / packet 처리]
    ↓
Session::Close
    ├─ socket shutdown / close
    └─ GameWorker::OnSessionClosed        ← 같은 스레드에서 정리
            ├─ User → UserManager::RemoveUser
            └─ Session → SessionManager::RemoveSession
    ↓
shared_ptr 참조 풀려서 자동 소멸
```

Close 트리거는 세 가지.

- 클라이언트가 disconnect (TCP FIN → recv 0 byte)
- 네트워크 에러 (recv error)
- 서버측 timeout (User::OnUpdate 에서 감지)

세 경우 모두 `Session::Close()` 가 단일 진입점이다. Close 안에서 GameWorker 의 `OnSessionClosed` 를 호출하면 GameWorker 스레드에서 User/Session 정리가 일어난다.

## 결합도와 설계 결정

### 양방향 참조 회피

`User` 가 `Session` 을 직접 보관하지 않는다. 대신 `User::OnUpdate()` 가 호출자(`GameWorker`)로부터 Session shared_ptr 을 인자로 받는다.

```cpp
m_user_manager->ForEachUser([this](const std::shared_ptr<User>& user)
{
    auto session = m_session_manager->GetSession(user->GetSessionId());
    user->OnUpdate(session);
});
```

User 가 SessionManager 를 직접 참조하면 결합도가 올라가고, 양방향 shared_ptr 이 되면 순환 참조 위험이 있다. 호출자가 lookup 후 넘겨주는 방식이 깔끔하다.

### ServerApp 의 역할 분리

`ServerApp` 한 클래스가 lifecycle 관리와 네트워크 reactor 역할을 겸한다. 별도의 NetworkReactor 클래스로 분리할 수도 있지만, asio 의 io_context 자체가 이미 reactor 인 상황에서 한 겹 더 감싸는 것이 가치 대비 코드량이 크다고 판단했다. 대신 멤버 변수를 lifecycle / network / workers 세 그룹으로 그룹핑해 책임을 명확히 보이게 했다.

### 풀 (object pool) 미도입

User 객체를 pool 로 관리할 수도 있지만, User 의 생성/소멸 빈도가 낮아 (분당 몇 건 수준) 풀 도입의 이득이 풀 관리 코드 복잡도를 정당화하지 못한다고 판단했다. Packet 이나 Job 처럼 hot path 에서 빈번하게 만들어지는 객체에는 풀이 의미 있지만, User 는 그 영역이 아니다.

## 부하 측정

단일 GameWorker 스레드로 게임 로직을 직렬화하는 이 설계가 실제로 부하를 견디는지 측정했다. 측정 대상은 MOVE 요청 처리다. 유저가 이동할 때마다 서버는 전체 유저를 순회해 (brute-force O(N)) 반경 내 인원을 세고 응답한다. 즉 초당 부하는 "요청 수 × 전체 유저 수" 에 비례한다.

측정은 asyncio 기반 봇(tests/loadbot.py)으로 N 개의 가상 유저를 접속시켜, 각자 초당 5회 MOVE 를 보내는 방식으로 했다. 동시 접속 폭주가 커널 accept 백로그를 넘겨 연결이 거부되는 것을 피하기 위해, 접속은 초당 일정 수로 나눠 분산(ramp-up)했다. 서버는 GameWorker 스레드 단독으로 job 처리량, 큐 깊이, 큐 대기 지연을 계측한다 (GS_ENABLE_METRICS 빌드).

측정 환경은 MacBook Air (M4) 단일 머신이며, 서버와 봇이 같은 CPU 자원을 공유하고 localhost 로 통신한다. 따라서 아래 수치는 절대 성능 벤치마크가 아니라 이 아키텍처의 "부하 반응 특성" 으로 읽어야 한다. 서버 전용 환경이나 봇을 별도 머신으로 분리하면 처리 상한은 더 높게 나올 수 있다.

| 동시 접속 | 목표 처리량 | 실제 처리량 | 큐 깊이 | 왕복 latency p99 |
|---|---|---|---|---|
| 1000 | 5000 job/s | 5000 (달성) | ~0 | 3.6 ms |
| 1500 | 7500 job/s | 7500 (달성) | ~0 | 7.6 ms |
| 2000 | 10000 job/s | ~7200 (포화) | ~600 상주 | 96.9 ms |

(처리량은 서버 계측 기준, latency 는 클라이언트 왕복 기준이다.)

결과의 요지.

- 단일 GameWorker 스레드는 약 7200 job/초까지 큐 없이 처리한다. 1500 동접 (7500 job/s) 까지는 큐가 거의 쌓이지 않고 p99 latency 가 8ms 이하로 유지된다.
- 2000 동접에서 유입이 처리 상한을 넘어서면 큐가 약 600 깊이에서 상주하고 latency p99 가 97ms 로 급증한다. 처리 한계에 도달한 것이다.
- 다만 큐는 무한히 발산하지 않는다. 요청-응답 구조상 서버가 밀리면 클라이언트도 다음 요청을 늦추게 되어, 큐가 일정 깊이에서 준평형을 이룬다. "Job 큐가 버퍼 역할을 한다" 는 설계 의도가 실제로 관측된 지점이다.

부하가 순전히 전체 유저 수에 비례하는 것은 AOI 조회가 brute-force 이기 때문이다. 공간 분할(grid) 을 도입하면 순회를 근처 영역으로 제한해 이 한계를 밀어낼 수 있는데, 그 경우 유저 밀도(맵 크기) 가 처리량에 영향을 주기 시작한다. 현재 brute-force 에서는 맵 크기가 부하에 영향을 주지 않는다.

## 의도적으로 제외한 것

- 실제 DB / Redis 연동
- TLS / 인증 시스템
- 분산 구조 (서버 간 통신, 로드밸런싱)
- 게임 컨텐츠 (전투, 인벤토리, 채팅 등)
- 공간 분할 기반 최적화 (grid/quadtree) 와 프로파일링 기반 성능 튜닝
- 패킷 직렬화 라이브러리 (FlatBuffers, Protobuf 등)

이들은 같은 워커 패턴 위에 추가될 수 있다는 것이 설계 의도다.

## 회고

이 샘플을 짜면서 의식적으로 따랐던 원칙.

- **추상화는 같은 패턴이 둘 이상 등장할 때만 도입**. Worker 베이스는 GameWorker / DbWorker 두 곳에서 쓰이므로 정당화된다. 단일 사용처를 위한 추상화는 피했다.
- **샘플의 가치는 "안 한 것" 도 보여준다**. 풀, 분리된 NetworkReactor, 별도 HeartbeatWorker 등 도입을 검토했지만 거절한 결정들이 있다. 도입 여부 자체보다 판단 근거가 더 중요하다.
- **샘플 규모에 맞는 단순화**. 도메인에서 흔히 쓰이는 패턴 중 다음 항목들은 의도적으로 더 단순한 형태로 만들었다.
    - libuv / 자체 reactor → asio io_context (이미 reactor 역할 제공)
    - 매크로 기반 싱글톤 → 명시적 의존성 주입
    - 다종의 메시지 enum → 필요한 4종으로 축소