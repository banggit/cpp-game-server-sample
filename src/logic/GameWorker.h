#pragma once

#include "worker/Worker.h"
#include "net/SessionManager.h"
#include "net/PacketHandler.h"
#include "common/Job.h"
#include "common/Types.h"

#include <memory>

namespace gs
{

class GameWorker : public Worker
{
public:
    GameWorker(std::shared_ptr<SessionManager> in_session_manager);
    ~GameWorker();

    GameWorker(const GameWorker&) = delete;
    GameWorker& operator=(const GameWorker&) = delete;

    void RemoveSession(SessionId in_session_id);

private:
    void OnCreate() override;
    void OnDestroy() override;
    void ProcessJob(const Job& in_job) override;

    void ProcessPacketJob(const Job& in_job);

    std::shared_ptr<SessionManager>     m_session_manager;
    PacketHandler                       m_packet_handler;
};

} // namespace gs