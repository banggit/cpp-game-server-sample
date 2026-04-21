#pragma once

#include "common/Types.h"

#include <boost/asio.hpp>

namespace gs
{

class Listener
{
public:
    Listener(boost::asio::io_context& in_io, Port in_port);

    void Start();
    void Stop();

private:
    void DoAccept();

    boost::asio::io_context&        m_io;
    boost::asio::ip::tcp::acceptor  m_acceptor;
    Port                            m_port;
};

} // namespace gs