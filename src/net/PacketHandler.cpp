#include "net/PacketHandler.h"

#include "log/Logger.h"

#include <cstring>

namespace gs
{

PacketHandler::PacketHandler()
{
}

bool PacketHandler::Dispatch(SessionId in_session_id, const std::vector<std::uint8_t>& in_packet_data)
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
        case PacketId::LOGIN:
            HandleLogin(in_session_id, payload, payload_size);
            return true;
        case PacketId::ECHO:
            HandleEcho(in_session_id, payload, payload_size);
            return true;
        case PacketId::HEARTBEAT:
            HandleHeartbeat(in_session_id, payload, payload_size);
            return true;
        default:
            LOG_WARN("session " + std::to_string(in_session_id)
                     + " unknown opcode: " + std::to_string(opcode));
            return false;
    }
}

void PacketHandler::HandleLogin(SessionId in_session_id, const std::uint8_t* in_payload, std::size_t in_payload_size)
{
    LOG_INFO("session " + std::to_string(in_session_id) + " LOGIN packet received");
}

void PacketHandler::HandleEcho(SessionId in_session_id, const std::uint8_t* in_payload, std::size_t in_payload_size)
{
    LOG_DEBUG("session " + std::to_string(in_session_id)
              + " ECHO packet received, size: " + std::to_string(in_payload_size));
}

void PacketHandler::HandleHeartbeat(SessionId in_session_id, const std::uint8_t* in_payload, std::size_t in_payload_size)
{
    LOG_DEBUG("session " + std::to_string(in_session_id) + " HEARTBEAT packet received");
}

} // namespace gs