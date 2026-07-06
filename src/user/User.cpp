#include "user/User.h"

#include "log/Logger.h"
#include "net/Session.h"

namespace gs
{

void User::OnUpdate(const std::shared_ptr<Session>& in_session)
{
    CheckHeartbeat(in_session);
}

void User::CheckHeartbeat(const std::shared_ptr<Session>& in_session)
{
    if (!in_session || !in_session->IsConnected())
    {
        return;
    }

    const auto now = std::chrono::steady_clock::now();
    const auto last_activity = in_session->GetLastActivity();
    const auto duration = std::chrono::duration_cast<std::chrono::seconds>(now - last_activity);

    if (duration > SESSION_TIMEOUT_DURATION)
    {
        LOG_INFO("user " + std::to_string(m_user_id)
                 + " (session " + std::to_string(in_session->GetSessionId())
                 + ") heartbeat timeout");
        in_session->Close();
    }
}

} // namespace gs