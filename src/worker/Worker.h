#pragma once

#include "common/Job.h"

#include <atomic>
#include <deque>
#include <memory>
#include <mutex>
#include <string>
#include <thread>

namespace gs
{

// 모든 워커의 베이스 클래스.
// 자체 스레드와 Job 큐를 소유하며, 등록 시점에 스레드가 시작된다.
// 종료는 WorkerManager가 일괄 처리한다.
class Worker : public std::enable_shared_from_this<Worker>
{
public:
    Worker();
    virtual ~Worker();

    Worker(const Worker&) = delete;
    Worker& operator=(const Worker&) = delete;

    void SetWorkerName(const std::string& in_name)
    {
        m_worker_name = in_name;
    }

    const std::string& GetWorkerName() const
    {
        return m_worker_name;
    }

    // Job 추가. 다른 스레드에서 호출 가능.
    void Push(const Job& in_job);
    void PushFront(const Job& in_job);

    // 큐에 적재된 Job 개수.
    std::size_t GetJobCount() const;

    // 워커 스레드 시작. WorkerManager::Insert() 시점에 호출됨.
    void StartWorkerThread();

    // 종료 신호. join은 호출자 책임.
    void Shutdown();

    bool IsJoinable() const
    {
        return m_thread.joinable();
    }

    void Join()
    {
        if (m_thread.joinable())
        {
            m_thread.join();
        }
    }

protected:
    // 워커 lifecycle hooks. 파생 클래스가 구현.
    virtual void OnCreate()  {}
    virtual void OnUpdate()  {}
    virtual void OnDestroy() {}

    // 매 루프마다 호출되는 디스패치.
    // 기본 구현은 큐에서 Job을 꺼내 ProcessJob() 호출.
    virtual void OnRun();

    // 실제 Job 처리. 파생 클래스가 구현.
    virtual void ProcessJob(const Job& in_job) = 0;

    Job Pop();
    bool HasJob() const;

private:
    void ThreadMain();

    std::deque<Job>                 m_queue;
    mutable std::mutex              m_mutex;
    std::thread                     m_thread;
    std::string                     m_worker_name;
    std::atomic<bool>               m_shutdown_requested;

    static constexpr int            DISPATCH_MAX_PER_LOOP = 1024;
    static constexpr int            SLEEP_MSEC = 1;
};

} // namespace gs