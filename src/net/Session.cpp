#include "net/Session.h"

#include "log/Logger.h"
#include "logic/GameWorker.h"
#include "common/Job.h"

namespace gs
{

Session::Session(boost::asio::io_context& in_io, SessionId in_session_id, std::shared_ptr<GameWorker> in_game_worker)
    : m_io(in_io)
    , m_socket(in_io)
    , m_session_id(in_session_id)
    , m_is_connected(false)
    , m_receive_buffer(MAX_PACKET_SIZE)
    , m_packet_buffer()
    , m_game_worker(in_game_worker)
    , m_user(nullptr)
    , m_send_queue()
    , m_is_sending(false)
    , m_last_activity_time(std::chrono::system_clock::now())
{
}

Session::~Session() = default;

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
    // 어떤 스레드에서 불려도 안전. 실제 정리는 io 스레드로 위임.
    auto self = shared_from_this();
    boost::asio::dispatch(m_io, [self]()
    {
        self->DoClose();
    });
}
    
void Session::DoClose()  // io_context 스레드에서만 실행
{
    if (!m_is_connected.load())
    {
        return;  // 중복 Close 요청 무시 — 이 판정도 io 스레드에서만 일어남
    }
    m_is_connected.store(false);

    boost::system::error_code ec;
    m_socket.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
    m_socket.close(ec);
    LOG_INFO("session " + std::to_string(m_session_id) + " closed");

    if (m_game_worker)
    {
        m_game_worker->Push(Job(JobType::SESSION_CLOSE, m_session_id, {}));
    }
}

void Session::SendPacket(const std::vector<std::uint8_t>& in_packet_data)
{
    if (!m_is_connected)
    {
        LOG_WARN("session " + std::to_string(m_session_id) + " send called on disconnected session");
        return;
    }

    // SendPacket은 GameWorker 스레드에서 호출되지만,
    // m_send_queue / m_is_sending / async_send 는 io_context 스레드에서만 만지도록 dispatch.
    // 이렇게 하면 락 없이도 single-threaded 보장으로 데이터 정합성이 유지된다.
    auto self = shared_from_this();
    boost::asio::post(m_io, [self, in_packet_data]()
    {
        if (!self->m_is_connected)
        {
            return;
        }

        self->m_send_queue.insert(self->m_send_queue.end(),
                                  in_packet_data.begin(),
                                  in_packet_data.end());

        if (!self->m_is_sending)
        {
            self->DoSend();
        }
    });
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

    m_packet_buffer.Append(m_receive_buffer.data(), in_bytes);
    ProcessPackets();

    DoReceive();
}

void Session::ProcessPackets()
{
    std::vector<std::uint8_t> packet_data;
    while (m_packet_buffer.TryReadPacket(packet_data))
    {
        Job job(JobType::PACKET_PROCESS, m_session_id, packet_data);
        m_game_worker->Push(job);
    }
}

void Session::DoSend()
{
    if (m_send_queue.empty() || m_is_sending || !m_is_connected)
    {
        return;
    }

    m_is_sending = true;
    auto self = shared_from_this();

    m_socket.async_send(
        boost::asio::buffer(m_send_queue),
        [self](const boost::system::error_code& in_ec, std::size_t in_bytes)
        {
            if (!self->m_is_connected)
            {
                return;
            }

            if (in_ec)
            {
                LOG_WARN("session " + std::to_string(self->m_session_id)
                         + " send error: " + in_ec.message());
                self->Close();
                return;
            }

            LOG_DEBUG("session " + std::to_string(self->m_session_id)
                      + " sent " + std::to_string(in_bytes) + " bytes");

            self->m_send_queue.erase(self->m_send_queue.begin(), self->m_send_queue.begin() + in_bytes);
            self->m_is_sending = false;

            if (!self->m_send_queue.empty())
            {
                self->DoSend();
            }
        });
}

} // namespace gs