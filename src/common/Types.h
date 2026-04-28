#pragma once

#include <cstdint>
#include <chrono>

namespace gs
{

using Port      = std::uint16_t;
using SessionId = std::uint64_t;
using UserId    = std::uint64_t;
using AccountId = std::uint64_t;

enum class PacketId : std::uint16_t
{
    LOGIN_REQ     = 1001,
    LOGIN_ACK     = 1002,
    ECHO_REQ      = 1003,
    ECHO_ACK      = 1004,
    HEARTBEAT_REQ = 1005,
    HEARTBEAT_ACK = 1006
};

constexpr std::size_t MAX_PACKET_SIZE = 4096;
constexpr std::size_t PACKET_HEADER_SIZE = 4;
constexpr auto SESSION_TIMEOUT_DURATION = std::chrono::seconds(30);

} // namespace gs