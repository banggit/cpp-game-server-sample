#pragma once

#include "worker/Worker.h"
#include "net/SessionManager.h"
#include "net/PacketHandler.h"
#include "common/Job.h"
#include "common/Types.h"

#include <chrono>
#include <memory>

#ifdef GS_ENABLE_METRICS
#include <array>
#include <cstdint>
#endif

namespace gs
{
    struct WorkerStats {
        std::uint64_t jobs_total = 0, last_total = 0;
        std::size_t   queue_depth_max = 0;
        std::array<std::uint64_t, 7> lat_buckets{};   // <0.1,<0.5,<1,<5,<10,<50,>=50 ms

        static std::size_t bucket_index(std::chrono::nanoseconds ns) {
            const double ms = std::chrono::duration<double, std::milli>(ns).count();
            if (ms < 0.1) return 0;
            if (ms < 0.5) return 1;
            if (ms < 1.0) return 2;
            if (ms < 5.0) return 3;
            if (ms < 10.0) return 4;
            if (ms < 50.0) return 5;
            return 6;
        }
        void record_latency(std::chrono::nanoseconds ns) { lat_buckets[bucket_index(ns)]++; }

        // 히스토그램 누적분포에서 백분위 근사 → 해당 버킷의 상한(ms)을 반환
        double percentile_upper_ms(double p) const {
            static constexpr double edges[7] = {0.1, 0.5, 1.0, 5.0, 10.0, 50.0, 1e9};
            std::uint64_t total = 0;
            for (auto c : lat_buckets) total += c;
            if (total == 0) return 0.0;
            const std::uint64_t target = static_cast<std::uint64_t>(total * p);
            std::uint64_t cum = 0;
            for (std::size_t i = 0; i < 7; ++i) {
                cum += lat_buckets[i];
                if (cum >= target) return edges[i];
            }
            return edges[6];
        }
    };

class DbWorker;
class UserManager;

class GameWorker : public Worker
{
public:
    GameWorker(std::shared_ptr<SessionManager> in_session_manager,
               std::shared_ptr<UserManager> in_user_manager);
    ~GameWorker();

    GameWorker(const GameWorker&) = delete;
    GameWorker& operator=(const GameWorker&) = delete;

    void SetDbWorker(std::shared_ptr<DbWorker> in_db_worker);
    void OnSessionClosed(SessionId in_session_id);

private:
    void OnCreate() override;
    void OnUpdate() override;
    void OnDestroy() override;
    void ProcessJob(const Job& in_job) override;

    void ProcessPacketJob(const Job& in_job);
    void UpdateAllUsers();

    std::shared_ptr<SessionManager>     m_session_manager;
    std::shared_ptr<UserManager>        m_user_manager;
    std::shared_ptr<DbWorker>           m_db_worker;
    PacketHandler                       m_packet_handler;

    std::chrono::system_clock::time_point   m_last_user_update;

    static constexpr auto USER_UPDATE_INTERVAL = std::chrono::seconds(5);
    
#ifdef GS_ENABLE_METRICS
    WorkerStats                              m_stats;
    std::chrono::system_clock::time_point    m_last_stats{std::chrono::system_clock::now()};
#endif
};

} // namespace gs