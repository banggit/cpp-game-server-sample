#pragma once

#include <cstdint>
#include <chrono>

namespace gs
{

using Port      = std::uint16_t;
using SessionId = std::uint64_t;

enum class PacketId : std::uint16_t
{
    LOGIN     = 1001,
    ECHO      = 1002,
    HEARTBEAT = 1003
};

constexpr std::size_t MAX_PACKET_SIZE = 4096;
constexpr auto SESSION_TIMEOUT_DURATION = std::chrono::seconds(30);

} // namespace gs