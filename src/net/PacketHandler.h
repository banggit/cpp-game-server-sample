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

// 패킷 처리에 필요한 컨텍스트를 한 묶음으로 전달.
// 모든 핸들러가 같은 시그니처를 갖게 하여 일관성을 높이고,
// 추후 핸들러에 정보가 추가될 때도 시그니처를 바꾸지 않게 한다.
struct PacketContext
{
    SessionId                            SessionId;
    const std::vector<std::uint8_t>&     PacketData;
    std::shared_ptr<Session>             Session;
};

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

    bool Dispatch(const PacketContext& in_ctx);

private:
    void HandleLogin(const PacketContext& in_ctx);
    void HandleEcho(const PacketContext& in_ctx);
    void HandleHeartbeat(const PacketContext& in_ctx);

    std::shared_ptr<DbWorker>       m_db_worker;
    std::shared_ptr<UserManager>    m_user_manager;
};

} // namespace gs