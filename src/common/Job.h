#pragma once

#include "common/Types.h"

#include <cstdint>
#include <vector>
#include <memory>

namespace gs
{

enum class JobType : std::uint8_t
{
    PACKET_PROCESS = 1
};

struct Job
{
    JobType                         Type;
    SessionId                       TargetSessionId;
    std::vector<std::uint8_t>       PacketData;

    Job(JobType in_type, SessionId in_session_id, const std::vector<std::uint8_t>& in_packet_data)
        : Type(in_type)
        , TargetSessionId(in_session_id)
        , PacketData(in_packet_data)
    {
    }
};

} // namespace gs