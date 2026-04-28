# C++ Game Server Sample

C++ 게임 서버의 기본 골격을 담은 샘플 프로젝트.

실무 게임 서버에서 자주 등장하는 구조 — 워커 기반 잡 디스패치, 네트워크/로직 스레드 분리, 세션 생명주기 관리, 비동기 DB I/O — 를 작은 규모로 정리해 보여주는 것이 목적이다.

설계 의도와 의사결정 배경은 [doc/architecture.md](doc/architecture.md) 참조.

## 기술 스택

- C++17
- CMake (3.16+)
- Boost.Asio (헤더 온리, 1.71+)
- pthread / std::thread

## 프로젝트 구조

```
src/
├── main.cpp                          # 엔트리 포인트
├── app/
│   └── ServerApp                     # 서버 lifecycle, 시그널, 워커 wiring
├── net/
│   ├── Listener                      # TCP accept 루프
│   ├── Session                       # 소켓 / 수신 / 송신 / 패킷 조립
│   ├── SessionManager                # 세션 컨테이너
│   ├── PacketBuffer                  # 수신 데이터 조립
│   ├── PacketBuilder                 # 응답 패킷 빌더
│   └── PacketHandler                 # opcode 분기 + 핸들러
├── logic/
│   ├── GameWorker                    # 게임 로직 전용 워커 스레드
│   └── DbWorker                      # DB I/O 전용 워커 스레드 (Mock)
├── user/
│   ├── User                          # 게임 내 유저 객체
│   └── UserManager                   # 활성 유저 컨테이너
├── worker/
│   ├── Worker                        # 워커 베이스 (스레드 + Job 큐)
│   └── WorkerManager                 # 워커 등록 / 일괄 종료
├── log/
│   └── Logger                        # 간단한 stdout 로거
└── common/
    ├── Types                         # 공통 타입 / 상수 / 패킷 ID
    └── Job                           # Job 구조체
```

## 빌드

### macOS

```bash
brew install boost cmake

cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j
```

### Windows

vcpkg 사용:

```bash
git clone https://github.com/Microsoft/vcpkg.git
cd vcpkg
.\bootstrap-vcpkg.bat
.\vcpkg install boost-asio:x64-windows

cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug ^
    -DCMAKE_TOOLCHAIN_FILE="path/to/vcpkg/scripts/buildsystems/vcpkg.cmake"
cmake --build build -j
```

Visual Studio 2022는 CMake 네이티브 지원이 있어 폴더를 열면 바로 빌드된다.

## 실행

```bash
# 기본 포트 7777
./build/game_server

# 다른 포트 지정
./build/game_server 8888
```

서버 부팅 시 워커 두 개 (game_worker, db_worker) 가 각자 스레드를 시작하고, 메인 스레드는 asio io_context 로 네트워크 reactor 역할을 한다.

종료는 Ctrl+C (SIGINT) 또는 SIGTERM. 워커는 일괄 종료된 후 메인 스레드가 빠진다.

## 패킷 포맷

```
Header (4 bytes)
  [opcode: uint16, little-endian]
  [payload_size: uint16, little-endian]
Payload (variable)
  [data: payload_size bytes]
```

### 정의된 패킷

| Opcode | Name           | Direction | Payload                          |
|--------|----------------|-----------|----------------------------------|
| 1001   | LOGIN_REQ      | C → S     | account_id (uint64)              |
| 1002   | LOGIN_ACK      | S → C     | result (uint8) + user_id (uint64)|
| 1003   | ECHO_REQ       | C → S     | arbitrary bytes                  |
| 1004   | ECHO_ACK       | S → C     | same as request payload          |
| 1005   | HEARTBEAT_REQ  | C → S     | (empty)                          |
| 1006   | HEARTBEAT_ACK  | S → C     | (empty)                          |

## 테스트

`tests/` 폴더에 Python 표준 라이브러리만 사용하는 테스트 클라이언트가 있다.

```bash
# 정상 로그인
python3 tests/client.py login --account 1000

# 메시지 echo
python3 tests/client.py echo --message "hello"

# 중복 로그인 거부 검증
python3 tests/client.py duplicate --account 1000

# 서버측 timeout 검증 (30초 idle)
python3 tests/client.py timeout --account 1000

# 대화형 모드
python3 tests/client.py interactive
```

자세한 사용법은 [tests/README.md](tests/README.md) 참조.

## 코드 컨벤션

- 클래스명: `PascalCase` (C 접두사 없음)
- 함수명: `PascalCase`
- 멤버 변수: `m_` + `snake_case`
- 매개변수: `in_` 접두사 (`out_` 도 동일 패턴)
- struct 멤버: `PascalCase` (접두사 없음)
- 상수: `UPPER_SNAKE_CASE` + `constexpr`
- enum class: `UPPER_SNAKE_CASE`
- 중괄호: Allman 스타일

## 의도적으로 제외한 것

샘플의 초점을 흐리지 않기 위해 의도적으로 뺀 항목.

- 실제 DB / Redis 연동 (Mock 워커로 흐름만 시연)
- TLS / 인증
- 분산 구조 / 서버 간 통신
- 게임 컨텐츠 (전투, 인벤토리 등)
- 부하 테스트 / 성능 튜닝

이들은 같은 워커 패턴 위에 추가될 수 있다는 것이 설계 의도다 — `PacketHandler` 에 opcode 추가, `Worker` 상속한 새 워커 (RedisWorker, MatchingWorker 등) 등록.