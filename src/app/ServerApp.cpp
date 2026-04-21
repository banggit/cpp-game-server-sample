#include "ServerApp.h"

#include "log/Logger.h"
#include "net/Listener.h"
#include "net/SessionManager.h"

#include <csignal>

namespace gs
{

ServerApp::ServerApp(Port in_port)
    : m_port(in_port)
    , m_io()
    , m_handlers(m_io, SIGINT, SIGTERM)
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

    m_session_manager = std::make_shared<SessionManager>(m_io);
    m_listener = std::make_unique<Listener>(m_io, m_port, m_session_manager);
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
    m_io.stop();
}

} // namespace gs