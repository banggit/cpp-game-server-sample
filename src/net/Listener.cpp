#include "net/Listener.h"

#include "log/Logger.h"
#include "net/SessionManager.h"
#include "logic/JobQueue.h"

#include <boost/system/error_code.hpp>

namespace gs
{

using boost::asio::ip::tcp;

Listener::Listener(boost::asio::io_context& in_io, Port in_port, std::shared_ptr<SessionManager> in_session_manager, std::shared_ptr<JobQueue> in_job_queue)
    : m_io(in_io)
    , m_acceptor(in_io)
    , m_port(in_port)
    , m_session_manager(in_session_manager)
    , m_job_queue(in_job_queue)
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
                auto session = m_session_manager->CreateSession(m_job_queue);
                session->Start(std::move(in_socket));
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