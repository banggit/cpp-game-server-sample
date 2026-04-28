#pragma once

#include "worker/Worker.h"
#include "net/SessionManager.h"
#include "net/PacketHandler.h"
#include "common/Job.h"
#include "common/Types.h"

#include <chrono>
#include <memory>

namespace gs
{

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
};

} // namespace gs