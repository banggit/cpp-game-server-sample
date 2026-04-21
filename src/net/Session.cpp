#include "net/Session.h"

#include "log/Logger.h"

namespace gs
{

Session::Session(boost::asio::io_context& in_io, SessionId in_session_id)
    : m_io(in_io)
    , m_socket(in_io)
    , m_session_id(in_session_id)
    , m_is_connected(false)
    , m_receive_buffer(MAX_PACKET_SIZE)
    , m_last_activity_time(std::chrono::system_clock::now())
{
}

Session::~Session()
{
    Close();
}

void Session::Start(boost::asio::ip::tcp::socket in_socket)
{
    m_socket = std::move(in_socket);
    m_is_connected = true;
    m_last_activity_time = std::chrono::system_clock::now();

    boost::system::error_code ec;
    const auto remote_ep = m_socket.remote_endpoint(ec);
    if (!ec)
    {
        LOG_INFO("session " + std::to_string(m_session_id)
                 + " started: " + remote_ep.address().to_string()
                 + ":" + std::to_string(remote_ep.port()));
    }

    DoReceive();
}

void Session::Close()
{
    if (m_is_connected)
    {
        m_is_connected = false;
        boost::system::error_code ec;
        m_socket.close(ec);
        LOG_INFO("session " + std::to_string(m_session_id) + " closed");
    }
}

void Session::DoReceive()
{
    if (!m_is_connected)
    {
        return;
    }

    auto self = shared_from_this();
    m_socket.async_receive(
        boost::asio::buffer(m_receive_buffer),
        [self](const boost::system::error_code& in_ec, std::size_t in_bytes)
        {
            self->OnReceiveComplete(in_ec, in_bytes);
        });
}

void Session::OnReceiveComplete(const boost::system::error_code& in_ec, std::size_t in_bytes)
{
    if (!m_is_connected)
    {
        return;
    }

    if (in_ec)
    {
        if (in_ec != boost::asio::error::operation_aborted)
        {
            LOG_WARN("session " + std::to_string(m_session_id)
                     + " receive error: " + in_ec.message());
        }
        Close();
        return;
    }

    if (in_bytes == 0)
    {
        LOG_INFO("session " + std::to_string(m_session_id) + " closed by peer");
        Close();
        return;
    }

    m_last_activity_time = std::chrono::system_clock::now();
    LOG_DEBUG("session " + std::to_string(m_session_id)
              + " received " + std::to_string(in_bytes) + " bytes");

    DoReceive();
}

} // namespace gs