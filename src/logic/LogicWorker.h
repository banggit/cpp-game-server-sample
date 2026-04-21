#pragma once

#include "logic/JobQueue.h"
#include "net/SessionManager.h"
#include "net/PacketHandler.h"

#include <thread>
#include <memory>

namespace gs
{

class LogicWorker
{
public:
    LogicWorker(std::shared_ptr<JobQueue> in_job_queue, std::shared_ptr<SessionManager> in_session_manager);
    ~LogicWorker();

    LogicWorker(const LogicWorker&) = delete;
    LogicWorker& operator=(const LogicWorker&) = delete;

    void Start();
    void Stop();

private:
    void Run();

    std::shared_ptr<JobQueue>           m_job_queue;
    std::shared_ptr<SessionManager>     m_session_manager;
    std::unique_ptr<std::thread>        m_thread;
    PacketHandler                       m_packet_handler;
    bool                                m_is_running;
};

} // namespace gs