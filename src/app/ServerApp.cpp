#include "app/ServerApp.h"

#include "log/Logger.h"
#include "net/Listener.h"
#include "net/SessionManager.h"
#include "logic/GameWorker.h"
#include "logic/DbWorker.h"
#include "logic/HeartbeatTimer.h"
#include "worker/WorkerManager.h"

#include <csignal>

namespace gs
{

ServerApp::ServerApp(Port in_port)
    : m_port(in_port)
    , m_worker_manager(std::make_shared<WorkerManager>())
    , m_io()
    , m_handlers(m_io, SIGINT, SIGTERM)
    , m_session_manager(std::make_shared<SessionManager>(m_io))
    , m_listener(nullptr)
    , m_game_worker(nullptr)
    , m_db_worker(nullptr)
    , m_heartbeat_timer(nullptr)
{
}

ServerApp::~ServerApp() = default;

// ────────────────────────────────────────────
// lifecycle
// ────────────────────────────────────────────
void ServerApp::Run()
{
    InitSignalHandlers();
    InitWorkers();
    InitNetwork();

    LOG_INFO("server running on port " + std::to_string(m_port));

    // 메인 스레드는 여기서 네트워크 I/O reactor로 동작한다.
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

// ────────────────────────────────────────────
// workers
// ────────────────────────────────────────────
void ServerApp::InitWorkers()
{
    // 1. GameWorker 등록.
    m_game_worker = std::make_shared<GameWorker>(m_session_manager);
    if (!m_worker_manager->Insert("game_worker", m_game_worker))
    {
        LOG_ERROR("failed to register game worker");
        return;
    }

    // 2. DbWorker 등록 (GameWorker 참조 주입).
    m_db_worker = std::make_shared<DbWorker>(m_game_worker);
    if (!m_worker_manager->Insert("db_worker", m_db_worker))
    {
        LOG_ERROR("failed to register db worker");
        return;
    }

    // 3. GameWorker가 DbWorker를 참조하도록 wiring.
    m_game_worker->SetDbWorker(m_db_worker);
}

// ────────────────────────────────────────────
// network reactor
// ────────────────────────────────────────────
void ServerApp::InitNetwork()
{
    m_heartbeat_timer = std::make_unique<HeartbeatTimer>(m_session_manager);
    m_heartbeat_timer->Start();

    m_listener = std::make_unique<Listener>(m_io, m_port, m_session_manager, m_game_worker);
    m_listener->Start();
}

} // namespace gs