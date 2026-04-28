#include "net/PacketHandler.h"

#include "log/Logger.h"
#include "net/Session.h"
#include "net/PacketBuilder.h"

#include <cstring>

namespace gs
{

PacketHandler::PacketHandler()
{
}

bool PacketHandler::Dispatch(SessionId in_session_id, const std::vector<std::uint8_t>& in_packet_data,
                             std::shared_ptr<Session> in_session)
{
    if (in_packet_data.size() < PACKET_HEADER_SIZE)
    {
        LOG_WARN("packet data too small");
        return false;
    }

    std::uint16_t opcode = 0;
    std::uint16_t payload_size = 0;

    std::memcpy(&opcode, in_packet_data.data(), sizeof(std::uint16_t));
    std::memcpy(&payload_size, in_packet_data.data() + sizeof(std::uint16_t), sizeof(std::uint16_t));

    const std::uint8_t* payload = in_packet_data.data() + PACKET_HEADER_SIZE;

    switch (static_cast<PacketId>(opcode))
    {
        case PacketId::LOGIN_REQ:
            HandleLogin(in_session_id, payload, payload_size, in_session);
            return true;
        case PacketId::ECHO_REQ:
            HandleEcho(in_session_id, payload, payload_size, in_session);
            return true;
        case PacketId::HEARTBEAT_REQ:
            HandleHeartbeat(in_session_id, payload, payload_size, in_session);
            return true;
        default:
            LOG_WARN("session " + std::to_string(in_session_id)
                     + " unknown opcode: " + std::to_string(opcode));
            return false;
    }
}

void PacketHandler::HandleLogin(SessionId in_session_id, const std::uint8_t* in_payload, std::size_t in_payload_size,
                                std::shared_ptr<Session> in_session)
{
    LOG_INFO("session " + std::to_string(in_session_id) + " LOGIN_REQ packet received");

    PacketBuilder response(PacketId::LOGIN_ACK);
    std::uint8_t result = 1;
    response.Write(&result, 1);

    in_session->SendPacket(response.Build());
}

void PacketHandler::HandleEcho(SessionId in_session_id, const std::uint8_t* in_payload, std::size_t in_payload_size,
                               std::shared_ptr<Session> in_session)
{
    LOG_DEBUG("session " + std::to_string(in_session_id)
              + " ECHO_REQ packet received, size: " + std::to_string(in_payload_size));

    PacketBuilder response(PacketId::ECHO_ACK);
    response.Write(in_payload, in_payload_size);

    in_session->SendPacket(response.Build());
}

void PacketHandler::HandleHeartbeat(SessionId in_session_id, const std::uint8_t* in_payload, std::size_t in_payload_size,
                                    std::shared_ptr<Session> in_session)
{
    LOG_DEBUG("session " + std::to_string(in_session_id) + " HEARTBEAT_REQ packet received");

    PacketBuilder response(PacketId::HEARTBEAT_ACK);
    in_session->SendPacket(response.Build());
}


} // namespace gs