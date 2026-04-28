#include "app/ServerApp.h"

#include "log/Logger.h"
#include "net/Listener.h"
#include "net/SessionManager.h"
#include "logic/GameWorker.h"
#include "logic/HeartbeatTimer.h"
#include "worker/WorkerManager.h"

#include <csignal>

namespace gs
{

ServerApp::ServerApp(Port in_port)
    : m_port(in_port)
    , m_io()
    , m_handlers(m_io, SIGINT, SIGTERM)
    , m_worker_manager(std::make_shared<WorkerManager>())
    , m_session_manager(std::make_shared<SessionManager>(m_io))
    , m_game_worker(nullptr)
    , m_heartbeat_timer(nullptr)
    , m_listener(nullptr)
{
}

ServerApp::~ServerApp() = default;

void ServerApp::InitSignalHandlers()
{
    m_handlers.async_wait([this](const boost::system::error_code& in_ec, int in_sig)
    {
        if (!in_ec)
        {
            LOG_INFO("signal received: " + std::to_string(in_sig) + ", shutting down");
            Stop();
        }
    });
}

void ServerApp::Run()
{
    InitSignalHandlers();

    m_game_worker = std::make_shared<GameWorker>(m_session_manager);
    if (!m_worker_manager->Insert("game_worker", m_game_worker))
    {
        LOG_ERROR("failed to register game worker");
        return;
    }

    m_heartbeat_timer = std::make_unique<HeartbeatTimer>(m_session_manager);
    m_heartbeat_timer->Start();

    m_listener = std::make_unique<Listener>(m_io, m_port, m_session_manager, m_game_worker);
    m_listener->Start();

    LOG_INFO("server running on port " + std::to_string(m_port));
    m_io.run();
    LOG_INFO("server stopped");
}

void ServerApp::Stop()
{
    if (m_listener)
    {
        m_listener->Stop();
    }
    if (m_heartbeat_timer)
    {
        m_heartbeat_timer->Stop();
    }
    if (m_game_worker)
    {
        m_game_worker->Shutdown();
    }
    if (m_worker_manager)
    {
        m_worker_manager->Destroy();
    }
    m_io.stop();
}

} // namespace gs