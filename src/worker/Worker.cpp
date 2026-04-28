#include "worker/Worker.h"

#include "log/Logger.h"

#include <chrono>

namespace gs
{

Worker::Worker()
    : m_queue()
    , m_mutex()
    , m_thread()
    , m_worker_name()
    , m_shutdown_requested(false)
{
}

Worker::~Worker()
{
    if (m_thread.joinable())
    {
        m_thread.join();
    }
}

void Worker::Push(const Job& in_job)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_queue.push_back(in_job);
}

void Worker::PushFront(const Job& in_job)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_queue.push_front(in_job);
}

std::size_t Worker::GetJobCount() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_queue.size();
}

bool Worker::HasJob() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return !m_queue.empty();
}

Job Worker::Pop()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_queue.empty())
    {
        return Job();
    }
    Job job = m_queue.front();
    m_queue.pop_front();
    return job;
}

void Worker::StartWorkerThread()
{
    m_thread = std::thread([this]()
    {
        ThreadMain();
    });
}

void Worker::Shutdown()
{
    m_shutdown_requested.store(true);
}

void Worker::ThreadMain()
{
    LOG_INFO("worker [" + m_worker_name + "] started");

    OnCreate();

    // 종료 요청이 와도 큐에 남은 Job은 처리한 후에 빠져나간다.
    while (!m_shutdown_requested.load() || HasJob())
    {
        OnUpdate();
        OnRun();

        // 큐가 비어있으면 짧게 sleep해서 busy loop 방지.
        if (!HasJob())
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(SLEEP_MSEC));
        }
    }

    OnDestroy();

    LOG_INFO("worker [" + m_worker_name + "] stopped");
}

void Worker::OnRun()
{
    for (int i = 0; i < DISPATCH_MAX_PER_LOOP; ++i)
    {
        if (!HasJob())
        {
            break;
        }

        const Job job = Pop();
        ProcessJob(job);
    }
}

} // namespace gs