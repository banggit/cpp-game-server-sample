#pragma once

#include "common/Types.h"

#include <chrono>
#include <memory>
#include <string>

namespace gs
{

class Session;

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
    
    void SetPos(float x, float y) { m_x = x; m_y = y; }
    float GetX() const { return m_x; }
    float GetY() const { return m_y; }

    // 주기적으로 GameWorker가 호출하는 갱신 함수.
    // Session은 호출자(GameWorker)가 SessionManager에서 lookup해서 넘긴다.
    // User가 SessionManager를 알 필요 없게 결합도를 낮춘다.
    void OnUpdate(const std::shared_ptr<Session>& in_session);

private:
    void CheckHeartbeat(const std::shared_ptr<Session>& in_session);

    UserId          m_user_id;
    AccountId       m_account_id;
    std::string     m_name;
    SessionId       m_session_id;
    
    float m_x = 0.0f;
    float m_y = 0.0f;
};

} // namespace gs