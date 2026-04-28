#pragma once

#include "common/Types.h"
#include "user/User.h"

#include <map>
#include <memory>
#include <mutex>

namespace gs
{

// 활성 유저 컨테이너.
// LOGIN 시점에 User를 생성/등록하고, 세션 종료 시 제거한다.
//
// account_id → user_id 매핑은 실제 게임에서는 DB 조회로 가져오지만,
// 샘플에서는 단순 변환 (account_id 1000 → user_id 1) 으로 시뮬한다.
class UserManager
{
public:
    UserManager();
    ~UserManager();

    UserManager(const UserManager&) = delete;
    UserManager& operator=(const UserManager&) = delete;

    // 메모리에 즉시 User 생성. LOGIN 처리 시 호출.
    std::shared_ptr<User> CreateUser(AccountId in_account_id, SessionId in_session_id);

    // user_id로 조회.
    std::shared_ptr<User> GetUser(UserId in_user_id);

    // 세션 종료 시 제거.
    void RemoveUser(UserId in_user_id);

    std::size_t GetUserCount() const;

    // account_id → user_id 변환 (샘플용 mock 매핑).
    static UserId ResolveUserId(AccountId in_account_id);

private:
    std::map<UserId, std::shared_ptr<User>>     m_users;
    mutable std::mutex                          m_mutex;
};

} // namespace gs