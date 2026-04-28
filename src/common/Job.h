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
    DB_QUERY        = 3,
    DB_CALLBACK     = 4
};

enum class DbResult : std::uint8_t
{
    SUCCESS = 0,
    FAILED  = 1
};

struct Job
{
    JobType                         Type;
    SessionId                       TargetSessionId;
    std::vector<std::uint8_t>       PacketData;
    DbResult                        Result;

    Job()
        : Type(JobType::PACKET_PROCESS)
        , TargetSessionId(0)
        , PacketData()
        , Result(DbResult::SUCCESS)
    {
    }

    Job(JobType in_type, SessionId in_session_id, const std::vector<std::uint8_t>& in_packet_data)
        : Type(in_type)
        , TargetSessionId(in_session_id)
        , PacketData(in_packet_data)
        , Result(DbResult::SUCCESS)
    {
    }

    using SharedPtr = std::shared_ptr<Job>;
    using WeakPtr   = std::weak_ptr<Job>;
};

} // namespace gs