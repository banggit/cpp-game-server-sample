# GameServerSample

C++ 샘플 서버 프로젝트입니다.

이 레포는 풀 MMORPG 서버가 아니라, 실무형 게임 서버 코드에서 자주 등장하는 **기본 골격**을 작게 정리해 보여주는 것을 목표로 합니다.

## 특징

- **네트워크/로직 스레드 분리**: async I/O + JobQueue 기반
- **패킷 처리 파이프라인**: 수신 → 조립 → opcode 분기 → 로직 스레드 처리
- **세션 생명주기 관리**: SessionManager + HeartbeatTimer
- **타임아웃 자동 처리**: 30초 이상 활동 없는 세션 자동 종료
- **크로스 플랫폼**: Windows (Visual Studio) / macOS (VSCode/Xcode) 동시 지원

## 기술 스택

- **C++17**
- **CMake** (빌드 시스템)
- **Boost.Asio** (비동기 네트워크)
- **Threads** (멀티스레드)

## 프로젝트 구조
src/
├── main.cpp                          # 엔트리 포인트
├── app/
│   ├── ServerApp.h / .cpp            # 서버 부팅/조립/정리
├── net/
│   ├── Listener.h / .cpp             # TCP accept 루프
│   ├── Session.h / .cpp              # 소켓/버퍼/수신 루프
│   ├── SessionManager.h / .cpp       # 세션 생명주기 관리
│   ├── PacketBuffer.h / .cpp         # 수신 데이터 조립
│   └── PacketHandler.h / .cpp        # opcode 기반 분기
├── logic/
│   ├── JobQueue.h / .cpp             # 스레드 안전 작업 큐
│   ├── LogicWorker.h / .cpp          # 로직 전용 스레드
│   └── HeartbeatTimer.h / .cpp       # 타임아웃 감시
├── log/
│   └── Logger.h                      # 간단한 로거
└── common/
├── Types.h                       # 공통 타입/상수
└── Job.h                         # Job 구조체

## 빌드

### macOS
```bash
# Homebrew로 Boost 설치 (처음 한 번만)
brew install boost cmake

# 빌드
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j
```

### Windows
```bash
# vcpkg로 Boost 설치 (처음 한 번만)
git clone https://github.com/Microsoft/vcpkg.git
cd vcpkg
.\bootstrap-vcpkg.bat
.\vcpkg install boost-asio:x64-windows

# 빌드
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug -DCMAKE_TOOLCHAIN_FILE="path/to/vcpkg/scripts/buildsystems/vcpkg.cmake"
cmake --build build -j
```

## 실행

```bash
# 기본 포트 7777로 시작
./build/game_server

# 다른 포트 지정
./build/game_server 8888

# 접속 테스트
nc 127.0.0.1 7777
telnet 127.0.0.1 7777
```

## 패킷 포맷
Header (4 bytes):
[opcode: 2 bytes (little-endian)]
[payload_size: 2 bytes (little-endian)]
Payload (variable):
[data: payload_size bytes]

### 샘플 패킷

**LOGIN** (opcode 1001)
```bash
echo -ne '\xe9\x03\x00\x00' | nc 127.0.0.1 7777
```

**ECHO** (opcode 1002, payload "test")
```bash
echo -ne '\xea\x03\x04\x00test' | nc 127.0.0.1 7777
```

**HEARTBEAT** (opcode 1003)
```bash
echo -ne '\xeb\x03\x00\x00' | nc 127.0.0.1 7777
```

다중 패킷 한 번에 전송:
```bash
(echo -ne '\xe9\x03\x00\x00\xea\x03\x04\x00test\xeb\x03\x00\x00'; sleep 1) | nc 127.0.0.1 7777
```

## 설계 핵심

### 1. 네트워크/로직 분리
- **네트워크 스레드** (boost::asio I/O context)
  - 소켓 수신
  - 패킷 조립
  - 작업을 JobQueue에 enqueue
- **로직 스레드** (LogicWorker)
  - JobQueue에서 꺼낸 작업 처리
  - 게임 로직 실행
  - 네트워크 콜백에서 직접 로직 실행 안 함

### 2. 세션 생명주기
accept → Session 생성 → 수신 루프
↓
(30초 타임아웃)
↓
HeartbeatTimer 감시 → 타임아웃 종료
↓
SessionManager에서 제거

### 3. 패킷 처리 파이프라인
Raw bytes → PacketBuffer (조립)
↓
완성 패킷 → Job (enqueue)
↓
LogicWorker (dequeue)
↓
PacketHandler (dispatch)
↓
opcode별 핸들러 실행

## 로그

로그는 stdout으로 출력되며, 타임스탬프와 레벨을 포함합니다:
15:30:45.123 [INFO ] server running on port 7777
15:30:47.456 [DEBUG] session 1 created
15:30:47.789 [INFO ] session 1 started: 127.0.0.1:54321
15:30:50.012 [DEBUG] session 1 received 4 bytes
15:30:50.345 [INFO ] session 1 LOGIN packet received
15:31:20.678 [INFO ] session 1 timeout detected
15:31:20.901 [INFO ] session 1 closed

## 확장 가능성

현재 샘플에서는 의도적으로 제외한 것들:

- **DB/Redis**: 게임 로직에만 집중하기 위해 제거
- **TLS**: 보안 프로토콜은 나중 과제
- **분산 구조**: 서버 간 통신, 로드밸런싱 제외
- **실제 게임 컨텐츠**: 로그인, 캐릭터 선택, 전투 로직 등

이들은 같은 구조를 기반으로 추가 가능합니다:
- `PacketHandler`에 새 opcode 추가
- `LogicWorker` 패킷 처리 로직 확장
- DB 쿼리 결과를 Job으로 처리

## 코드 컨벤션

- **클래스명**: PascalCase (예: `ServerApp`, `SessionManager`)
- **함수명**: PascalCase (예: `Init`, `Start`, `Stop`)
- **멤버 변수**: `m_` + snake_case (예: `m_session_id`, `m_is_connected`)
- **매개변수**: `in_` prefix (예: `in_port`, `in_session`)
- **struct 멤버**: PascalCase, prefix 없음 (예: `Type`, `SessionId`)
- **상수**: 대문자 snake_case, `constexpr` (예: `MAX_PACKET_SIZE`)
- **enum class**: PascalCase (예: `PacketId`, `JobType`)

## 참고

이것은 **C++ 서버 샘플**입니다. 실무 게임 서버는 더 복잡한 구조를 가지지만, 이 코드의 핵심:
- 세션 관리
- 패킷 처리
- 스레드 분리
- 타임아웃 처리

등은 모든 게임 서버의 기초입니다.