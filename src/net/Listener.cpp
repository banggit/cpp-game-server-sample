
#include "Listener.h"

#include "log/Logger.h"

#include <boost/system/error_code.hpp>

namespace gs
{

using boost::asio::ip::tcp;

Listener::Listener(boost::asio::io_context& in_io, Port in_port)
    : m_io(in_io)
    , m_acceptor(in_io)
    , m_port(in_port)
{
}

void Listener::Start()
{
    boost::system::error_code ec;
    tcp::endpoint ep(tcp::v4(), m_port);

    m_acceptor.open(ep.protocol(), ec);
    if (ec)
    {
        LOG_ERROR("acceptor open failed: " + ec.message());
        return;
    }

    m_acceptor.set_option(boost::asio::socket_base::reuse_address(true), ec);
    m_acceptor.bind(ep, ec);
    if (ec)
    {
        LOG_ERROR("acceptor bind failed: " + ec.message());
        return;
    }

    m_acceptor.listen(boost::asio::socket_base::max_listen_connections, ec);
    if (ec)
    {
        LOG_ERROR("acceptor listen failed: " + ec.message());
        return;
    }

    LOG_INFO("listener started on :" + std::to_string(m_port));
    DoAccept();
}

void Listener::Stop()
{
    boost::system::error_code ec;
    m_acceptor.close(ec);
    LOG_INFO("listener stopped");
}

void Listener::DoAccept()
{
    m_acceptor.async_accept(
        [this](const boost::system::error_code& in_ec, tcp::socket in_socket)
        {
            if (!in_ec)
            {
                boost::system::error_code ep_ec;
                const auto ep = in_socket.remote_endpoint(ep_ec);
                if (!ep_ec)
                {
                    LOG_INFO("accepted: " + ep.address().to_string()
                             + ":" + std::to_string(ep.port()));
                }
                else
                {
                    LOG_INFO("accepted: <unknown endpoint>");
                }
            }
            else if (in_ec != boost::asio::error::operation_aborted)
            {
                LOG_WARN("accept error: " + in_ec.message());
            }

            if (m_acceptor.is_open())
            {
                DoAccept();
            }
        });
}

} // namespace gs