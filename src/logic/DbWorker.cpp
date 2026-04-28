#include "logic/DbWorker.h"

#include "logic/GameWorker.h"
#include "log/Logger.h"

#include <chrono>
#include <thread>

namespace gs
{

DbWorker::DbWorker(std::shared_ptr<GameWorker> in_game_worker)
    : Worker()
    , m_game_worker(in_game_worker)
{
}

DbWorker::~DbWorker() = default;

void DbWorker::OnCreate()
{
    LOG_DEBUG("db worker created");
}

void DbWorker::OnDestroy()
{
    LOG_DEBUG("db worker destroyed");
}

void DbWorker::ProcessJob(const Job& in_job)
{
    switch (in_job.Type)
    {
        case JobType::DB_QUERY:
            ProcessDbQuery(in_job);
            break;

        default:
            LOG_WARN("db worker unknown job type: " + std::to_string(static_cast<int>(in_job.Type)));
            break;
    }
}

void DbWorker::ProcessDbQuery(const Job& in_job)
{
    LOG_DEBUG("db worker processing query for session " + std::to_string(in_job.TargetSessionId));

    // 실제 DB I/O 시뮬레이션.
    std::this_thread::sleep_for(DB_LATENCY);

    // 결과를 GameWorker로 콜백.
    Job callback_job;
    callback_job.Type            = JobType::DB_CALLBACK;
    callback_job.TargetSessionId = in_job.TargetSessionId;
    callback_job.PacketData      = in_job.PacketData;
    callback_job.Result          = DbResult::SUCCESS;

    if (m_game_worker)
    {
        m_game_worker->Push(callback_job);
    }
}

} // namespace gs