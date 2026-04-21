#include "app/ServerApp.h"

#include "log/Logger.h"
#include "net/Listener.h"
#include "net/SessionManager.h"
#include "logic/JobQueue.h"
#include "logic/LogicWorker.h"

#include <csignal>

namespace gs
{

ServerApp::ServerApp(Port in_port)
    : m_port(in_port)
    , m_io()
    , m_handlers(m_io, SIGINT, SIGTERM)
    , m_job_queue(std::make_shared<JobQueue>())
    , m_session_manager(std::make_shared<SessionManager>(m_io))
    , m_logic_worker(nullptr)
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

    m_logic_worker = std::make_unique<LogicWorker>(m_job_queue, m_session_manager);
    m_logic_worker->Start();

    m_listener = std::make_unique<Listener>(m_io, m_port, m_session_manager, m_job_queue);
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
    if (m_logic_worker)
    {
        m_logic_worker->Stop();
    }
    m_io.stop();
}

} // namespace gs