#pragma once

#include "worker/Worker.h"

#include <atomic>
#include <map>
#include <memory>
#include <mutex>
#include <string>

namespace gs
{

// 워커 등록/조회/소멸을 관리하는 중앙 컨테이너.
// 등록 시점에 워커 스레드가 시작되고, Destroy() 시점에 일괄 종료한다.
class WorkerManager
{
public:
    WorkerManager();
    ~WorkerManager();

    WorkerManager(const WorkerManager&) = delete;
    WorkerManager& operator=(const WorkerManager&) = delete;

    bool Insert(const std::string& in_name, std::shared_ptr<Worker> in_worker);

    std::shared_ptr<Worker> Find(const std::string& in_name);

    template<typename T>
    std::shared_ptr<T> FindAs(const std::string& in_name)
    {
        return std::dynamic_pointer_cast<T>(Find(in_name));
    }

    void Destroy();

    std::size_t GetWorkerCount() const;

private:
    std::map<std::string, std::shared_ptr<Worker>>  m_workers;
    mutable std::mutex                              m_mutex;
    std::atomic<bool>                               m_destroyed;
};

} // namespace gs