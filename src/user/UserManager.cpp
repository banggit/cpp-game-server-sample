#include "user/UserManager.h"

#include "log/Logger.h"

namespace gs
{

UserManager::UserManager()
    : m_users()
{
}

UserManager::~UserManager()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_users.clear();
}

UserId UserManager::ResolveUserId(AccountId in_account_id)
{
    // 실제 게임에서는 DB의 account 테이블에서 user_id를 조회한다.
    // 샘플에서는 단순 변환: account_id 1000 → user_id 1, 1001 → 2, ...
    constexpr AccountId ACCOUNT_ID_BASE = 1000;
    if (in_account_id < ACCOUNT_ID_BASE)
    {
        return 0;
    }
    return static_cast<UserId>(in_account_id - ACCOUNT_ID_BASE + 1);
}

std::shared_ptr<User> UserManager::CreateUser(AccountId in_account_id, SessionId in_session_id)
{
    const UserId user_id = ResolveUserId(in_account_id);
    if (user_id == 0)
    {
        LOG_WARN("user manager: invalid account_id " + std::to_string(in_account_id));
        return nullptr;
    }

    std::lock_guard<std::mutex> lock(m_mutex);

    // 이미 같은 user_id로 접속 중인 유저가 있다면 거부.
    // 실제 게임에서는 기존 세션을 끊고 새 세션을 받는 정책도 흔하다.
    if (m_users.find(user_id) != m_users.end())
    {
        LOG_WARN("user manager: user " + std::to_string(user_id) + " already online");
        return nullptr;
    }

    const std::string name = "user_" + std::to_string(user_id);
    auto user = std::make_shared<User>(user_id, in_account_id, name);
    user->SetSessionId(in_session_id);

    m_users[user_id] = user;
    LOG_INFO("user " + std::to_string(user_id) + " (" + name + ") created");

    return user;
}

std::shared_ptr<User> UserManager::GetUser(UserId in_user_id)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    auto it = m_users.find(in_user_id);
    if (it != m_users.end())
    {
        return it->second;
    }
    return nullptr;
}

void UserManager::RemoveUser(UserId in_user_id)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    auto it = m_users.find(in_user_id);
    if (it != m_users.end())
    {
        m_users.erase(it);
        LOG_INFO("user " + std::to_string(in_user_id) + " removed");
    }
}

std::size_t UserManager::GetUserCount() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_users.size();
}

} // namespace gs