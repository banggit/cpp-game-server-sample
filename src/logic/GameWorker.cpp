#include "logic/GameWorker.h"

#include "logic/DbWorker.h"
#include "log/Logger.h"
#include "net/Session.h"

namespace gs
{

GameWorker::GameWorker(std::shared_ptr<SessionManager> in_session_manager)
    : Worker()
    , m_session_manager(in_session_manager)
    , m_db_worker(nullptr)
    , m_packet_handler()
{
}

GameWorker::~GameWorker() = default;

void GameWorker::SetDbWorker(std::shared_ptr<DbWorker> in_db_worker)
{
    m_db_worker = in_db_worker;
    m_packet_handler.SetDbWorker(in_db_worker);
}

void GameWorker::OnCreate()
{
    LOG_DEBUG("game worker created");
}

void GameWorker::OnDestroy()
{
    LOG_DEBUG("game worker destroyed");
}

void GameWorker::RemoveSession(SessionId in_session_id)
{
    if (m_session_manager)
    {
        m_session_manager->RemoveSession(in_session_id);
    }
}

void GameWorker::ProcessJob(const Job& in_job)
{
    switch (in_job.Type)
    {
        case JobType::PACKET_PROCESS:
            ProcessPacketJob(in_job);
            break;

        case JobType::DB_CALLBACK:
            ProcessDbCallback(in_job);
            break;

        case JobType::SESSION_CLOSE:
            LOG_DEBUG("session close: " + std::to_string(in_job.TargetSessionId));
            break;

        default:
            LOG_WARN("unknown job type: " + std::to_string(static_cast<int>(in_job.Type)));
            break;
    }
}

void GameWorker::ProcessPacketJob(const Job& in_job)
{
    auto session = m_session_manager->GetSession(in_job.TargetSessionId);
    if (!session)
    {
        LOG_WARN("packet job: session not found [" + std::to_string(in_job.TargetSessionId) + "]");
        return;
    }

    m_packet_handler.Dispatch(in_job.TargetSessionId, in_job.PacketData, session);
}

void GameWorker::ProcessDbCallback(const Job& in_job)
{
    auto session = m_session_manager->GetSession(in_job.TargetSessionId);
    if (!session)
    {
        LOG_WARN("db callback: session not found [" + std::to_string(in_job.TargetSessionId) + "]");
        return;
    }

    m_packet_handler.OnDbCallback(in_job.TargetSessionId, in_job.PacketData, in_job.Result, session);
}

} // namespace gs