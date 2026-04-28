#include "app/ServerApp.h"

#include "log/Logger.h"
#include "net/Listener.h"
#include "net/SessionManager.h"
#include "logic/GameWorker.h"
#include "logic/DbWorker.h"
#include "user/UserManager.h"
#include "worker/WorkerManager.h"

#include <csignal>

namespace gs
{

ServerApp::ServerApp(Port in_port)
    : m_port(in_port)
    , m_worker_manager(std::make_shared<WorkerManager>())
    , m_user_manager(std::make_shared<UserManager>())
    , m_io()
    , m_handlers(m_io, SIGINT, SIGTERM)
    , m_session_manager(std::make_shared<SessionManager>(m_io))
    , m_listener(nullptr)
    , m_game_worker(nullptr)
    , m_db_worker(nullptr)
{
}

ServerApp::~ServerApp() = default;

void ServerApp::Run()
{
    InitSignalHandlers();
    InitWorkers();
    InitNetwork();

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
    m_worker_manager->Destroy();
    m_io.stop();
}

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

void ServerApp::InitWorkers()
{
    m_game_worker = std::make_shared<GameWorker>(m_session_manager, m_user_manager);
    if (!m_worker_manager->Insert("game_worker", m_game_worker))
    {
        LOG_ERROR("failed to register game worker");
        return;
    }

    m_db_worker = std::make_shared<DbWorker>(m_game_worker);
    if (!m_worker_manager->Insert("db_worker", m_db_worker))
    {
        LOG_ERROR("failed to register db worker");
        return;
    }

    m_game_worker->SetDbWorker(m_db_worker);
}

void ServerApp::InitNetwork()
{
    m_listener = std::make_unique<Listener>(m_io, m_port, m_session_manager, m_game_worker);
    m_listener->Start();
}

} // namespace gs