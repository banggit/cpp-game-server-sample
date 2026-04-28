#pragma once

#include "worker/Worker.h"

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

    // 워커 등록. 스레드가 즉시 시작된다.
    bool Insert(const std::string& in_name, std::shared_ptr<Worker> in_worker);

    // 등록된 워커 조회.
    std::shared_ptr<Worker> Find(const std::string& in_name);

    template<typename T>
    std::shared_ptr<T> FindAs(const std::string& in_name)
    {
        return std::dynamic_pointer_cast<T>(Find(in_name));
    }

    // 모든 워커에 종료 신호 후 join.
    void Destroy();

    std::size_t GetWorkerCount() const;

private:
    std::map<std::string, std::shared_ptr<Worker>>  m_workers;
    mutable std::mutex                              m_mutex;
    bool                                            m_destroyed;
};

} // namespace gs