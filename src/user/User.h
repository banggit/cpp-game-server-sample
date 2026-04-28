#pragma once

#include "common/Types.h"

#include <string>

namespace gs
{

// 게임 내 유저(캐릭터) 객체.
// LOGIN 시 메모리에 즉시 생성되며, Session에 바인딩된다.
// 세션 종료 시 UserManager에서 제거되어 자동 소멸한다.
class User
{
public:
    User(UserId in_user_id, AccountId in_account_id, const std::string& in_name)
        : m_user_id(in_user_id)
        , m_account_id(in_account_id)
        , m_name(in_name)
        , m_session_id(0)
    {
    }

    UserId GetUserId() const
    {
        return m_user_id;
    }

    AccountId GetAccountId() const
    {
        return m_account_id;
    }

    const std::string& GetName() const
    {
        return m_name;
    }

    SessionId GetSessionId() const
    {
        return m_session_id;
    }

    void SetSessionId(SessionId in_session_id)
    {
        m_session_id = in_session_id;
    }

private:
    UserId          m_user_id;
    AccountId       m_account_id;
    std::string     m_name;
    SessionId       m_session_id;
};

} // namespace gs