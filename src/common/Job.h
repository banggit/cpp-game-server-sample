#pragma once

#include "common/Types.h"

#include <cstdint>
#include <vector>
#include <memory>

namespace gs
{

enum class JobType : std::uint8_t
{
    PACKET_PROCESS  = 1,
    SESSION_CLOSE   = 2,
    DB_QUERY        = 3,
    DB_CALLBACK     = 4
};

struct Job
{
    JobType                         Type;
    SessionId                       TargetSessionId;
    std::vector<std::uint8_t>       PacketData;

    Job()
        : Type(JobType::PACKET_PROCESS)
        , TargetSessionId(0)
        , PacketData()
    {
    }

    Job(JobType in_type, SessionId in_session_id, const std::vector<std::uint8_t>& in_packet_data)
        : Type(in_type)
        , TargetSessionId(in_session_id)
        , PacketData(in_packet_data)
    {
    }

    using SharedPtr = std::shared_ptr<Job>;
    using WeakPtr   = std::weak_ptr<Job>;
};

} // namespace gs