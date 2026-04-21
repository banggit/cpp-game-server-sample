# GameServerSample

게임 서버 개발자 취업용으로 작성한 C++ 샘플 서버 프로젝트입니다.

이 레포는 풀 MMORPG 서버가 아니라, 서버 코드에서 자주 등장하는 기본 골격을 작게 정리해 보여주는 것을 목표로 합니다.

## 목표

이 샘플은 아래 구조를 보여주는 데 초점을 맞춥니다.

- 서버 부팅 흐름
- Listener 기반 연결 수락
- Session / SessionManager 구조
- Packet 처리 구조
- JobQueue / LogicWorker 기반 로직 분리
- Heartbeat / Timeout 처리

현재는 초기 골격 단계이며, 점진적으로 기능을 추가하고 있습니다.

## 기술 스택

- C++17
- CMake
- Boost.Asio
- macOS / Windows 개발 환경 대응

## 프로젝트 구조

```text
src/
├── main.cpp
├── app/
│   ├── ServerApp.h
│   └── ServerApp.cpp
├── net/
│   ├── Listener.h
│   └── Listener.cpp
├── log/
│   └── Logger.h
└── common/
    └── Types.h