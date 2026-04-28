#include "net/PacketHandler.h"

#include "net/Session.h"
#include "net/PacketBuilder.h"
#include "logic/DbWorker.h"
#include "common/Types.h"
#include "log/Logger.h"

#include <cstring>

namespace gs
{

bool PacketHandler::Dispatch(SessionId in_session_id,
                             const std::vector<std::uint8_t>& in_packet_data,
                             std::shared_ptr<Session> in_session)
{
    if (in_packet_data.size() < PACKET_HEADER_SIZE)
    {
        LOG_WARN("packet too small from session " + std::to_string(in_session_id));
        return false;
    }

    std::uint16_t opcode = 0;
    std::memcpy(&opcode, in_packet_data.data(), sizeof(opcode));

    switch (static_cast<PacketId>(opcode))
    {
        case PacketId::LOGIN_REQ:
            HandleLogin(in_session_id, in_packet_data, in_session);
            break;

        case PacketId::ECHO_REQ:
            HandleEcho(in_session_id, in_packet_data, in_session);
            break;

        case PacketId::HEARTBEAT_REQ:
            HandleHeartbeat(in_session_id, in_packet_data, in_session);
            break;

        default:
            LOG_WARN("unknown opcode " + std::to_string(opcode)
                     + " from session " + std::to_string(in_session_id));
            return false;
    }

    return true;
}

void PacketHandler::HandleLogin(SessionId in_session_id,
                                const std::vector<std::uint8_t>& in_packet_data,
                                std::shared_ptr<Session> in_session)
{
    LOG_INFO("LOGIN_REQ from session " + std::to_string(in_session_id)
             + " -> dispatching to db worker");

    // DbWorker로 DB_QUERY Job 전송.
    // 결과는 DB_CALLBACK으로 GameWorker에 돌아온다.
    if (m_db_worker)
    {
        Job query_job(JobType::DB_QUERY, in_session_id, in_packet_data);
        m_db_worker->Push(query_job);
    }
    else
    {
        LOG_WARN("db worker not set, sending login ack directly");

        PacketBuilder response(PacketId::LOGIN_ACK);
        std::uint8_t result = 1;
        response.Write(&result, 1);
        in_session->SendPacket(response.Build());
    }
}

void PacketHandler::HandleEcho(SessionId in_session_id,
                               const std::vector<std::uint8_t>& in_packet_data,
                               std::shared_ptr<Session> in_session)
{
    LOG_DEBUG("ECHO_REQ from session " + std::to_string(in_session_id));

    // 페이로드만 추출해 그대로 응답.
    PacketBuilder response(PacketId::ECHO_ACK);
    if (in_packet_data.size() > PACKET_HEADER_SIZE)
    {
        const std::uint8_t* payload = in_packet_data.data() + PACKET_HEADER_SIZE;
        const std::size_t payload_size = in_packet_data.size() - PACKET_HEADER_SIZE;
        response.Write(payload, payload_size);
    }
    in_session->SendPacket(response.Build());
}

void PacketHandler::HandleHeartbeat(SessionId in_session_id,
                                    const std::vector<std::uint8_t>& in_packet_data,
                                    std::shared_ptr<Session> in_session)
{
    LOG_DEBUG("HEARTBEAT_REQ from session " + std::to_string(in_session_id));

    PacketBuilder response(PacketId::HEARTBEAT_ACK);
    in_session->SendPacket(response.Build());
}

void PacketHandler::OnDbCallback(SessionId in_session_id,
                                 const std::vector<std::uint8_t>& in_packet_data,
                                 DbResult in_result,
                                 std::shared_ptr<Session> in_session)
{
    LOG_INFO("db callback for session " + std::to_string(in_session_id)
             + " result: " + (in_result == DbResult::SUCCESS ? "success" : "failed"));

    PacketBuilder response(PacketId::LOGIN_ACK);
    std::uint8_t result = (in_result == DbResult::SUCCESS) ? 1 : 0;
    response.Write(&result, 1);
    in_session->SendPacket(response.Build());
}

} // namespace gs