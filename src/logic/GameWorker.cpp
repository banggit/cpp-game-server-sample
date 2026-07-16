#include "logic/GameWorker.h"

#include "logic/DbWorker.h"
#include "user/UserManager.h"
#include "user/User.h"
#include "net/Session.h"
#include "log/Logger.h"

namespace gs
{

GameWorker::GameWorker(std::shared_ptr<SessionManager> in_session_manager,
                       std::shared_ptr<UserManager> in_user_manager)
    : Worker()
    , m_session_manager(in_session_manager)
    , m_user_manager(in_user_manager)
    , m_db_worker(nullptr)
    , m_packet_handler()
    , m_last_user_update(std::chrono::system_clock::now())
{
    m_packet_handler.SetUserManager(in_user_manager);
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
    m_last_user_update = std::chrono::system_clock::now();
}

void GameWorker::OnDestroy()
{
    LOG_DEBUG("game worker destroyed");
}

void GameWorker::OnUpdate()
{
    // ThreadMain은 매 1ms 루프이므로,
    // 주기적 작업은 시간 기반으로 직접 throttle한다.
    const auto now = std::chrono::system_clock::now();
#ifdef GS_ENABLE_METRICS
    if (now - m_last_stats >= std::chrono::seconds(1)) {
        const auto d = GetJobCount();
        if (d > m_stats.queue_depth_max) m_stats.queue_depth_max = d;
        const auto rate = m_stats.jobs_total - m_stats.last_total;
        m_stats.last_total = m_stats.jobs_total;
        m_last_stats = now;
        LOG_INFO("[stats] jobs/s=" + std::to_string(rate)
                 + " q_now=" + std::to_string(d)
                 + " q_max=" + std::to_string(m_stats.queue_depth_max)
                 + " p50<=" + std::to_string(m_stats.percentile_upper_ms(0.50))
                 + " p95<=" + std::to_string(m_stats.percentile_upper_ms(0.95))
                 + " p99<=" + std::to_string(m_stats.percentile_upper_ms(0.99)) + "ms");
    }
#endif
        
    if (now - m_last_user_update < USER_UPDATE_INTERVAL)
    {
        return;
    }
    m_last_user_update = now;

    UpdateAllUsers();
}

void GameWorker::UpdateAllUsers()
{
    if (!m_user_manager || !m_session_manager)
    {
        return;
    }

    m_user_manager->ForEachUser([this](const std::shared_ptr<User>& in_user)
    {
        if (!in_user)
        {
            return;
        }

        // User에 바인딩된 세션을 찾아 함께 전달.
        // User가 SessionManager를 직접 참조하지 않게 하여 결합도를 낮춘다.
        auto session = m_session_manager->GetSession(in_user->GetSessionId());
        in_user->OnUpdate(session);
    });
}

void GameWorker::OnSessionClosed(SessionId in_session_id)
{
    if (!m_session_manager)
    {
        return;
    }

    auto session = m_session_manager->GetSession(in_session_id);
    if (session)
    {
        auto user = session->GetUser();
        if (user && m_user_manager)
        {
            m_user_manager->RemoveUser(user->GetUserId());
        }
    }

    m_session_manager->RemoveSession(in_session_id);
}

void GameWorker::ProcessJob(const Job& in_job)
{
    
#ifdef GS_ENABLE_METRICS
    ++m_stats.jobs_total;
    if (in_job.Type == JobType::PACKET_PROCESS)
        m_stats.record_latency(std::chrono::steady_clock::now() - in_job.EnqueuedAt);
#endif
    
    switch (in_job.Type)
    {
        case JobType::PACKET_PROCESS:
            ProcessPacketJob(in_job);
            break;

        case JobType::SESSION_CLOSE:
            // 항상 GameWorker 스레드에서만 정리 (User / Session 제거).
            OnSessionClosed(in_job.TargetSessionId);
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

    // PacketContext 로 패킷 처리 인자를 묶어서 핸들러에 전달.
    PacketContext ctx{ in_job.TargetSessionId, in_job.PacketData, session };
    m_packet_handler.Dispatch(ctx);
}

} // namespace gs