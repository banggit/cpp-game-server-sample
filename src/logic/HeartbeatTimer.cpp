#include "logic/HeartbeatTimer.h"

#include "log/Logger.h"
#include "common/Job.h"

#include <chrono>

namespace gs
{

HeartbeatTimer::HeartbeatTimer(std::shared_ptr<SessionManager> in_session_manager, std::shared_ptr<JobQueue> in_job_queue)
    : m_session_manager(in_session_manager)
    , m_job_queue(in_job_queue)
    , m_thread(nullptr)
    , m_is_running(false)
{
}

HeartbeatTimer::~HeartbeatTimer()
{
    Stop();
}

void HeartbeatTimer::Start()
{
    m_is_running = true;
    m_thread = std::make_unique<std::thread>([this]
    {
        Run();
    });
    LOG_INFO("heartbeat timer started");
}

void HeartbeatTimer::Stop()
{
    if (!m_is_running)
    {
        return;
    }

    m_is_running = false;

    if (m_thread && m_thread->joinable())
    {
        m_thread->join();
    }

    LOG_INFO("heartbeat timer stopped");
}

void HeartbeatTimer::Run()
{
    LOG_DEBUG("heartbeat timer thread running");

    while (m_is_running)
    {
        std::this_thread::sleep_for(HEARTBEAT_INTERVAL);

        if (!m_is_running)
        {
            break;
        }

        CheckSessionTimeout();
    }

    LOG_DEBUG("heartbeat timer thread exiting");
}

void HeartbeatTimer::CheckSessionTimeout()
{
    const auto now = std::chrono::system_clock::now();
    const auto session_count = m_session_manager->GetSessionCount();

    if (session_count == 0)
    {
        return;
    }

    std::vector<SessionId> sessions_to_close;

    for (SessionId i = 1; i <= 10000; ++i)
    {
        auto session = m_session_manager->GetSession(i);
        if (!session)
        {
            continue;
        }

        if (!session->IsConnected())
        {
            continue;
        }

        const auto last_activity = session->GetLastActivity();
        const auto duration = std::chrono::duration_cast<std::chrono::seconds>(now - last_activity);

        if (duration > SESSION_TIMEOUT_DURATION)
        {
            LOG_INFO("session " + std::to_string(i) + " timeout detected");
            sessions_to_close.push_back(i);
        }
    }

    for (SessionId session_id : sessions_to_close)
    {
        auto session = m_session_manager->GetSession(session_id);
        if (session)
        {
            session->Close();
            m_session_manager->RemoveSession(session_id);
        }
    }
}

} // namespace gs