#pragma once

#include "common/Types.h"
#include "common/Job.h"

#include <cstdint>
#include <memory>
#include <vector>

namespace gs
{

class Session;
class DbWorker;
class UserManager;

class PacketHandler
{
public:
    PacketHandler() = default;
    ~PacketHandler() = default;

    void SetDbWorker(std::shared_ptr<DbWorker> in_db_worker)
    {
        m_db_worker = in_db_worker;
    }

    void SetUserManager(std::shared_ptr<UserManager> in_user_manager)
    {
        m_user_manager = in_user_manager;
    }

    bool Dispatch(SessionId in_session_id,
                  const std::vector<std::uint8_t>& in_packet_data,
                  std::shared_ptr<Session> in_session);

private:
    void HandleLogin(SessionId in_session_id,
                     const std::vector<std::uint8_t>& in_packet_data,
                     std::shared_ptr<Session> in_session);

    void HandleEcho(SessionId in_session_id,
                    const std::vector<std::uint8_t>& in_packet_data,
                    std::shared_ptr<Session> in_session);

    void HandleHeartbeat(SessionId in_session_id,
                         const std::vector<std::uint8_t>& in_packet_data,
                         std::shared_ptr<Session> in_session);

    std::shared_ptr<DbWorker>       m_db_worker;
    std::shared_ptr<UserManager>    m_user_manager;
};

} // namespace gs