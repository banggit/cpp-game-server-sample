#include "logic/JobQueue.h"

#include "log/Logger.h"

namespace gs
{

JobQueue::JobQueue()
    : m_queue()
    , m_mutex()
    , m_cv()
    , m_is_shutdown(false)
{
}

JobQueue::~JobQueue()
{
    Shutdown();
}

void JobQueue::Enqueue(const Job& in_job)
{
    std::unique_lock<std::mutex> lock(m_mutex);
    if (m_is_shutdown)
    {
        LOG_WARN("enqueue called on shutdown queue");
        return;
    }
    m_queue.push(in_job);
    m_cv.notify_one();
}

bool JobQueue::TryDequeue(Job& out_job)
{
    std::unique_lock<std::mutex> lock(m_mutex);

    m_cv.wait(lock, [this]
    {
        return !m_queue.empty() || m_is_shutdown;
    });

    if (m_queue.empty())
    {
        return false;
    }

    out_job = m_queue.front();
    m_queue.pop();
    return true;
}

void JobQueue::Shutdown()
{
    std::unique_lock<std::mutex> lock(m_mutex);
    m_is_shutdown = true;
    m_cv.notify_all();
}

bool JobQueue::IsShutdown() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_is_shutdown;
}

} // namespace gs