#pragma once

#include "common/Types.h"

#include <boost/asio.hpp>
#include <memory>

namespace gs
{

class Listener;
class SessionManager;
class GameWorker;
class HeartbeatTimer;
class WorkerManager;

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

    Port                                m_port;
    boost::asio::io_context             m_io;
    boost::asio::signal_set             m_handlers;
    std::shared_ptr<WorkerManager>      m_worker_manager;
    std::shared_ptr<SessionManager>     m_session_manager;
    std::shared_ptr<GameWorker>         m_game_worker;
    std::unique_ptr<HeartbeatTimer>     m_heartbeat_timer;
    std::unique_ptr<Listener>           m_listener;
};

} // namespace gs