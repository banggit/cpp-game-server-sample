#include "logic/DbWorker.h"

#include "logic/GameWorker.h"
#include "log/Logger.h"

#include <chrono>
#include <thread>

namespace gs
{

DbWorker::DbWorker(std::weak_ptr<GameWorker> in_game_worker)
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
        case JobType::DB_LOG_LOGIN:
            ProcessDbQuery(in_job);
            break;

        default:
            LOG_WARN("db worker unknown job type: " + std::to_string(static_cast<int>(in_job.Type)));
            break;
    }
}

void DbWorker::ProcessDbQuery(const Job& in_job)
{
    LOG_DEBUG("db worker logging login for session " + std::to_string(in_job.TargetSessionId));

    // 실제 DB INSERT 시뮬레이션.
    std::this_thread::sleep_for(DB_LATENCY);

    // fire-and-forget: 결과를 GameWorker에 돌려보내지 않는다.
    // 실패한다면 여기서 로그만 남기고 끝낸다 (실제 코드라면 retry 큐 등).
    LOG_DEBUG("db worker login record written for session " + std::to_string(in_job.TargetSessionId));
}

} // namespace gs