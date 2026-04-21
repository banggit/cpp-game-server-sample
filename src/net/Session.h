#pragma once

#include "common/Types.h"
#include "net/PacketBuffer.h"
#include "net/PacketHandler.h"

#include <boost/asio.hpp>
#include <memory>
#include <chrono>

namespace gs
{

class Session : public std::enable_shared_from_this<Session>
{
public:
    Session(boost::asio::io_context& in_io, SessionId in_session_id);
    ~Session();

    Session(const Session&) = delete;
    Session& operator=(const Session&) = delete;

    void Start(boost::asio::ip::tcp::socket in_socket);
    void Close();

    SessionId GetSessionId() const
    {
        return m_session_id;
    }

    bool IsConnected() const
    {
        return m_is_connected;
    }

    std::chrono::system_clock::time_point GetLastActivity() const
    {
        return m_last_activity_time;
    }

private:
    void DoReceive();
    void OnReceiveComplete(const boost::system::error_code& in_ec, std::size_t in_bytes);
    void ProcessPackets();

    boost::asio::io_context&        m_io;
    boost::asio::ip::tcp::socket    m_socket;
    SessionId                        m_session_id;
    bool                            m_is_connected;
    std::vector<std::uint8_t>       m_receive_buffer;
    PacketBuffer                    m_packet_buffer;
    PacketHandler                   m_packet_handler;
    std::chrono::system_clock::time_point m_last_activity_time;
};

} // namespace gs