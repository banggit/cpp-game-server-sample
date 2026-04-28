#pragma once

#include "common/Types.h"

#include <cstdint>
#include <vector>
#include <memory>

namespace gs
{

class Session;

class PacketHandler
{
public:
    PacketHandler();

    bool Dispatch(SessionId in_session_id, const std::vector<std::uint8_t>& in_packet_data, 
                  std::shared_ptr<Session> in_session);

private:
    void HandleLogin(SessionId in_session_id, const std::uint8_t* in_payload, std::size_t in_payload_size,
                     std::shared_ptr<Session> in_session);
    void HandleEcho(SessionId in_session_id, const std::uint8_t* in_payload, std::size_t in_payload_size,
                    std::shared_ptr<Session> in_session);
    void HandleHeartbeat(SessionId in_session_id, const std::uint8_t* in_payload, std::size_t in_payload_size,
                         std::shared_ptr<Session> in_session);
};

} // namespace gs