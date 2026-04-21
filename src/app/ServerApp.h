#pragma once

#include "common/Types.h"

#include <boost/asio.hpp>
#include <memory>

namespace gs
{

class Listener;
class SessionManager;

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
    std::shared_ptr<SessionManager>     m_session_manager;
    std::unique_ptr<Listener>           m_listener;
};

} // namespace gs