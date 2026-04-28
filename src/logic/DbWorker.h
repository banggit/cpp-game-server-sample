#pragma once

#include "worker/Worker.h"
#include "common/Job.h"

#include <memory>

namespace gs
{

class GameWorker;

// Mock DB 워커.
// 실제 DB 없이 sleep으로 I/O를 시뮬레이션한다.
// 작업 완료 후 결과를 GameWorker에 DB_CALLBACK Job으로 돌려보낸다.
class DbWorker : public Worker
{
public:
    explicit DbWorker(std::shared_ptr<GameWorker> in_game_worker);
    ~DbWorker();

    DbWorker(const DbWorker&) = delete;
    DbWorker& operator=(const DbWorker&) = delete;

private:
    void OnCreate() override;
    void OnDestroy() override;
    void ProcessJob(const Job& in_job) override;

    void ProcessDbQuery(const Job& in_job);

    std::shared_ptr<GameWorker>     m_game_worker;

    static constexpr auto DB_LATENCY = std::chrono::milliseconds(50);
};

} // namespace gs