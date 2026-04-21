#pragma once

#include "common/Types.h"

#include <cstdint>
#include <vector>

namespace gs
{

class PacketHandler
{
public:
    PacketHandler();

    bool Dispatch(SessionId in_session_id, const std::vector<std::uint8_t>& in_packet_data);

private:
    void HandleLogin(SessionId in_session_id, const std::uint8_t* in_payload, std::size_t in_payload_size);
    void HandleEcho(SessionId in_session_id, const std::uint8_t* in_payload, std::size_t in_payload_size);
    void HandleHeartbeat(SessionId in_session_id, const std::uint8_t* in_payload, std::size_t in_payload_size);
};

} // namespace gs