#pragma once

#include "common/Types.h"

#include <boost/asio.hpp>
#include <memory>

namespace gs
{

class SessionManager;
class GameWorker;

class Listener
{
public:
    Listener(boost::asio::io_context& in_io, Port in_port, std::shared_ptr<SessionManager> in_session_manager, std::shared_ptr<GameWorker> in_game_worker);

    void Start();
    void Stop();

private:
    void DoAccept();

    boost::asio::io_context&                m_io;
    boost::asio::ip::tcp::acceptor          m_acceptor;
    Port                                    m_port;
    std::shared_ptr<SessionManager>         m_session_manager;
    std::shared_ptr<GameWorker>             m_game_worker;
};

} // namespace gs