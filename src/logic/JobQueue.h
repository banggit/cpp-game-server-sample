#pragma once

#include "common/Job.h"

#include <queue>
#include <mutex>
#include <condition_variable>
#include <memory>

namespace gs
{

class JobQueue
{
public:
    JobQueue();
    ~JobQueue();

    void Enqueue(const Job& in_job);
    bool TryDequeue(Job& out_job);
    void Shutdown();

    bool IsShutdown() const;

private:
    std::queue<Job>             m_queue;
    mutable std::mutex          m_mutex;
    std::condition_variable     m_cv;
    bool                        m_is_shutdown;
};

} // namespace gs