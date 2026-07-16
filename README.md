# C++ Game Server Sample

[![CI](https://github.com/banggit/cpp-game-server-sample/actions/workflows/ci.yml/badge.svg)](https://github.com/banggit/cpp-game-server-sample/actions/workflows/ci.yml)

C++ 게임 서버의 기본 골격을 담은 샘플 프로젝트.

실무 게임 서버에서 자주 등장하는 구조 — 워커 기반 잡 디스패치, 네트워크/로직 스레드 분리, 세션 생명주기 관리, 비동기 DB I/O — 를 작은 규모로 정리해 보여주는 것이 목적이다.

설계 의도와 의사결정 배경은 [doc/architecture.md](doc/architecture.md) 참조.

## 기술 스택

- C++17
- CMake (3.19+ — `CMakePresets.json` v3 사용)
- Boost.Asio (헤더 온리)
- std::thread

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

이 프로젝트는 `CMakePresets.json`으로 빌드 설정을 관리한다. 빌드 명령어는 OS 무관하게 동일한 형식이고, 각 OS의 toolchain / generator 차이는 preset이 흡수한다.

### 사전 준비

**macOS**

```bash
brew install boost cmake
```

**Linux** (Ubuntu / Debian)

```bash
sudo apt-get install libboost-dev cmake build-essential
```

**Windows**

vcpkg 설치 + 환경변수 설정 (한 번만):

```powershell
git clone https://github.com/Microsoft/vcpkg.git C:\vcpkg
cd C:\vcpkg
.\bootstrap-vcpkg.bat
.\vcpkg install boost-asio:x64-windows

[System.Environment]::SetEnvironmentVariable('VCPKG_ROOT', 'C:\vcpkg', 'User')
```

설정 후 PowerShell / VSCode 재시작 필요.

### 빌드 명령

```bash
# macOS
cmake --preset macos-debug
cmake --build --preset macos-debug

# Linux
cmake --preset linux-debug
cmake --build --preset linux-debug

# Windows (PowerShell)
cmake --preset windows-debug
cmake --build --preset windows-debug
```

Release 빌드는 preset 이름을 `*-release`로 바꾼다.

### 사용 가능한 Preset

| Preset             | 설명                                |
|--------------------|-------------------------------------|
| `macos-debug`      | macOS Debug (Makefile generator)    |
| `macos-debug-metrics` | macOS Debug + 부하 계측 (GS_ENABLE_METRICS) |
| `macos-release`    | macOS Release                       |
| `linux-debug`      | Linux Debug                         |
| `linux-release`    | Linux Release                       |
| `windows-debug`    | Windows Debug (Visual Studio)       |
| `windows-release`  | Windows Release                     |

각 preset은 OS 조건이 걸려있어 다른 OS에서는 자동으로 보이지 않는다.

### 실행

```bash
# macOS / Linux
./build/game_server
./build/game_server 8888   # 포트 지정

# Windows (PowerShell)
.\build\Debug\game_server.exe
.\build\Debug\game_server.exe 8888
```

> Windows의 Visual Studio generator는 multi-config이므로 빌드 결과물이 `build\Debug\` 또는 `build\Release\` 서브폴더에 생성된다 (macOS / Linux의 Makefile generator와 다른 점).

## Docker

OS / 의존성 설치 없이 바로 빌드/실행 가능하다.

```bash
# 이미지 빌드
docker build -t cpp-game-server-sample .

# 실행 (기본 포트 7777)
docker run --rm -p 7777:7777 cpp-game-server-sample

# 다른 포트로 실행
docker run --rm -p 8888:8888 cpp-game-server-sample 8888
```

Multi-stage build로 빌드 환경(Boost dev, build-essential)과 런타임을 분리하여 최종 이미지 크기를 100MB대로 유지한다 (단일 stage 시 600MB 이상).

## VSCode 디버깅

`.vscode/launch.json`이 macOS / Windows 모두 지원하도록 OS별 override가 들어있다.

- macOS: CodeLLDB 확장 필요 (`vadimcn.vscode-lldb`)
- Windows: C/C++ 확장 필요 (`ms-vscode.cpptools`)

F5 누르면 빌드 후 디버거가 자동으로 붙는다 (`tasks.json`이 OS 자동 감지).

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
| 1007   | MOVE_REQ       | C → S     | x (float) + y (float)            |
| 1008   | MOVE_ACK       | S → C     | nearby_count (uint16)            |
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

# 부하 측정 봇 (계측 빌드 서버 대상)
python3 tests/loadbot.py --conns 1000 --rate 5 --duration 60 --ramp 100
```

자세한 사용법은 [tests/README.md](tests/README.md) 참조.

## 부하 측정

단일 GameWorker 스레드가 MOVE(AOI 조회) 부하를 어디까지 견디는지 측정했다. 측정 환경은 MacBook Air (M4) 단일 머신, 서버·봇이 자원을 공유하는 localhost 조건이라 절대 벤치마크가 아니라 부하 반응 특성으로 읽어야 한다.

| 동시 접속 | 목표 처리량 | 실제 처리량 | 큐 깊이 | p99 latency |
|---|---|---|---|---|
| 1000 | 5000 job/s | 5000 (달성) | ~0 | 3.6 ms |
| 1500 | 7500 job/s | 7500 (달성) | ~0 | 7.6 ms |
| 2000 | 10000 job/s | ~7200 (포화) | ~600 상주 | 96.9 ms |

단일 로직 스레드는 약 7200 job/초까지 큐 없이 처리하며(1500 동접, p99 8ms 이하), 그 이상에서 큐가 쌓이고 latency가 급증한다. 큐는 무한 발산하지 않고 준평형을 이뤄 버퍼 역할을 한다. 측정 방법·해석은 [doc/architecture.md](doc/architecture.md#부하-측정) 참고.

## CI

GitHub Actions로 매 push / PR 마다 자동 검증한다.

- macOS / Linux × Debug / Release 매트릭스 빌드 + smoke test
- Docker 이미지 빌드 + 컨테이너 smoke test

워크플로우: [.github/workflows/ci.yml](.github/workflows/ci.yml)

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
- 공간 분할 기반 최적화 (grid/quadtree), 프로파일링 기반 성능 튜닝

이들은 같은 워커 패턴 위에 추가될 수 있다는 것이 설계 의도다 — `PacketHandler` 에 opcode 추가, `Worker` 상속한 새 워커 (RedisWorker, MatchingWorker 등) 등록.