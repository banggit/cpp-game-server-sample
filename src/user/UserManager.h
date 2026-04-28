#pragma once

#include "common/Types.h"
#include "user/User.h"

#include <functional>
#include <map>
#include <memory>
#include <mutex>

namespace gs
{

class UserManager
{
public:
    UserManager();
    ~UserManager();

    UserManager(const UserManager&) = delete;
    UserManager& operator=(const UserManager&) = delete;

    std::shared_ptr<User> CreateUser(AccountId in_account_id, SessionId in_session_id);
    std::shared_ptr<User> GetUser(UserId in_user_id);
    void RemoveUser(UserId in_user_id);

    std::size_t GetUserCount() const;

    static UserId ResolveUserId(AccountId in_account_id);

    // 모든 User 순회.
    // 콜백 안에서 RemoveUser 등 컨테이너 변경이 일어날 수 있어
    // 락 안에서 스냅샷을 만들고 락 밖에서 콜백을 실행한다.
    using ForEachCallback = std::function<void(const std::shared_ptr<User>&)>;
    void ForEachUser(const ForEachCallback& in_callback);

private:
    std::map<UserId, std::shared_ptr<User>>     m_users;
    mutable std::mutex                          m_mutex;
};

} // namespace gs