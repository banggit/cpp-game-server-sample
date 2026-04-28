#pragma once

#include "common/Types.h"

#include <boost/asio.hpp>
#include <memory>

namespace gs
{

class Listener;
class SessionManager;
class GameWorker;
class DbWorker;
class HeartbeatTimer;
class WorkerManager;

// 서버 lifecycle을 관리하는 최상위 클래스.
//
// 책임:
//   1. lifecycle    - 시작/종료, 시그널 처리
//   2. network      - asio io_context (네트워크 reactor 역할)
//   3. workers      - GameWorker / DbWorker 등록 및 wiring
//
// io_context.run()이 메인 스레드에서 돌며 네트워크 I/O를 담당하고,
// 게임 로직은 GameWorker 스레드에서, DB I/O는 DbWorker 스레드에서 수행된다.
class ServerApp
{
public:
    explicit ServerApp(Port in_port);
    ~ServerApp();

    ServerApp(const ServerApp&) = delete;
    ServerApp& operator=(const ServerApp&) = delete;

    void Run();
    void Stop();

private:
    void InitSignalHandlers();
    void InitWorkers();
    void InitNetwork();

    // ────────────────────────────────────────────
    // lifecycle
    // ────────────────────────────────────────────
    Port                                m_port;
    std::shared_ptr<WorkerManager>      m_worker_manager;

    // ────────────────────────────────────────────
    // network reactor (asio I/O on main thread)
    // ────────────────────────────────────────────
    boost::asio::io_context             m_io;
    boost::asio::signal_set             m_handlers;
    std::shared_ptr<SessionManager>     m_session_manager;
    std::unique_ptr<Listener>           m_listener;

    // ────────────────────────────────────────────
    // workers (each runs on its own thread)
    // ────────────────────────────────────────────
    std::shared_ptr<GameWorker>         m_game_worker;
    std::shared_ptr<DbWorker>           m_db_worker;
    std::unique_ptr<HeartbeatTimer>     m_heartbeat_timer;
};

} // namespace gs