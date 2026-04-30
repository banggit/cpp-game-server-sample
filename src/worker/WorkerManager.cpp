#include "worker/WorkerManager.h"

#include "log/Logger.h"

namespace gs
{

WorkerManager::WorkerManager()
    : m_workers()
    , m_destroyed(false)
{
}

WorkerManager::~WorkerManager()
{
    Destroy();
}

bool WorkerManager::Insert(const std::string& in_name, std::shared_ptr<Worker> in_worker)
{
    if (!in_worker)
    {
        LOG_WARN("worker manager insert failed: null worker");
        return false;
    }

    {
        std::lock_guard<std::mutex> lock(m_mutex);

        if (m_workers.find(in_name) != m_workers.end())
        {
            LOG_WARN("worker manager insert failed: duplicated name [" + in_name + "]");
            return false;
        }

        m_workers[in_name] = in_worker;
    }

    in_worker->SetWorkerName(in_name);
    in_worker->StartWorkerThread();

    LOG_INFO("worker manager: registered [" + in_name + "]");
    return true;
}

std::shared_ptr<Worker> WorkerManager::Find(const std::string& in_name)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    auto it = m_workers.find(in_name);
    if (it == m_workers.end())
    {
        return nullptr;
    }
    return it->second;
}

void WorkerManager::Destroy()
{
    // 두 스레드가 동시에 진입해도 한 번만 실행되도록 CAS 로 진입 검사.
    bool expected = false;
    if (!m_destroyed.compare_exchange_strong(expected, true))
    {
        return;
    }

    std::map<std::string, std::shared_ptr<Worker>> snapshot;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        snapshot = m_workers;
    }

    // 1. 모든 워커에 종료 요청.
    for (auto& [name, worker] : snapshot)
    {
        if (worker)
        {
            worker->Shutdown();
        }
    }

    // 2. 모든 워커 join.
    for (auto& [name, worker] : snapshot)
    {
        if (worker && worker->IsJoinable())
        {
            worker->Join();
        }
    }

    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_workers.clear();
    }

    LOG_INFO("worker manager destroyed");
}

std::size_t WorkerManager::GetWorkerCount() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_workers.size();
}

} // namespace gs