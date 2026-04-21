#include "logic/LogicWorker.h"

#include "log/Logger.h"

namespace gs
{

LogicWorker::LogicWorker(std::shared_ptr<JobQueue> in_job_queue, std::shared_ptr<SessionManager> in_session_manager)
    : m_job_queue(in_job_queue)
    , m_session_manager(in_session_manager)
    , m_thread(nullptr)
    , m_packet_handler()
    , m_is_running(false)
{
}

LogicWorker::~LogicWorker()
{
    Stop();
}

void LogicWorker::Start()
{
    m_is_running = true;
    m_thread = std::make_unique<std::thread>([this]
    {
        Run();
    });
    LOG_INFO("logic worker started");
}

void LogicWorker::Stop()
{
    if (!m_is_running)
    {
        return;
    }

    m_is_running = false;
    m_job_queue->Shutdown();

    if (m_thread && m_thread->joinable())
    {
        m_thread->join();
    }

    LOG_INFO("logic worker stopped");
}

void LogicWorker::Run()
{
    LOG_DEBUG("logic worker thread running");

    Job job(JobType::PACKET_PROCESS, 0, std::vector<std::uint8_t>());
    while (m_job_queue->TryDequeue(job))
    {
        if (job.Type == JobType::PACKET_PROCESS)
        {
            const bool dispatch_result = m_packet_handler.Dispatch(job.TargetSessionId, job.PacketData);
            if (!dispatch_result)
            {
                LOG_WARN("logic worker dispatch failed for session " + std::to_string(job.TargetSessionId));
            }
        }
    }

    LOG_DEBUG("logic worker thread exiting");
}

} // namespace gs