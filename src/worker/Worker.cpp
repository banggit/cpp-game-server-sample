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
    // 정상 흐름에서는 WorkerManager가 Shutdown()을 호출하지만,
    // 예외 경로에서도 무한 join 되지 않도록 방어적으로 호출.
    // Shutdown()은 atomic store만 하므로 idempotent.
    Shutdown();

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
    // 람다가 self(shared_ptr)를 보유하므로 스레드 종료 전까지 Worker는 살아있음.
    // [this] 캡처 시 Worker 소멸 후 스레드가 dangling pointer 접근하는 위험을 방지.
    auto self = shared_from_this();
    m_thread = std::thread([self]()
    {
        self->ThreadMain();
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