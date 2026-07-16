#pragma once

#include "common/Types.h"

#include <cstdint>
#include <memory>
#include <vector>

namespace gs
{

enum class JobType : std::uint8_t
{
    PACKET_PROCESS  = 1,
    SESSION_CLOSE   = 2,
    DB_LOG_LOGIN    = 3   // fire-and-forget DB 기록
};

struct Job
{
    std::chrono::steady_clock::time_point EnqueuedAt = std::chrono::steady_clock::now();
    
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