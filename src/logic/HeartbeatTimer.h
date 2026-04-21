#pragma once

#include "common/Types.h"
#include "net/SessionManager.h"
#include "logic/JobQueue.h"

#include <thread>
#include <memory>
#include <chrono>

namespace gs
{

class HeartbeatTimer
{
public:
    HeartbeatTimer(std::shared_ptr<SessionManager> in_session_manager, std::shared_ptr<JobQueue> in_job_queue);
    ~HeartbeatTimer();

    HeartbeatTimer(const HeartbeatTimer&) = delete;
    HeartbeatTimer& operator=(const HeartbeatTimer&) = delete;

    void Start();
    void Stop();

private:
    void Run();
    void CheckSessionTimeout();

    std::shared_ptr<SessionManager>     m_session_manager;
    std::shared_ptr<JobQueue>           m_job_queue;
    std::unique_ptr<std::thread>        m_thread;
    bool                                m_is_running;

    static constexpr auto HEARTBEAT_INTERVAL = std::chrono::seconds(5);
};

} // namespace gs