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
class WorkerManager;
class UserManager;

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
    std::shared_ptr<UserManager>        m_user_manager;

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
};

} // namespace gs