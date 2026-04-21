#include "net/SessionManager.h"

#include "log/Logger.h"
#include "logic/JobQueue.h"

namespace gs
{

SessionManager::SessionManager(boost::asio::io_context& in_io)
    : m_io(in_io)
    , m_next_session_id(1)
{
}

SessionManager::~SessionManager()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_sessions.clear();
}

std::shared_ptr<Session> SessionManager::CreateSession(std::shared_ptr<JobQueue> in_job_queue)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    const SessionId session_id = m_next_session_id++;
    auto session = std::make_shared<Session>(m_io, session_id, in_job_queue);

    m_sessions[session_id] = session;
    LOG_DEBUG("session " + std::to_string(session_id) + " created");

    return session;
}

void SessionManager::RemoveSession(SessionId in_session_id)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    auto it = m_sessions.find(in_session_id);
    if (it != m_sessions.end())
    {
        m_sessions.erase(it);
        LOG_DEBUG("session " + std::to_string(in_session_id) + " removed from manager");
    }
}

std::shared_ptr<Session> SessionManager::GetSession(SessionId in_session_id)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    auto it = m_sessions.find(in_session_id);
    if (it != m_sessions.end())
    {
        return it->second;
    }
    return nullptr;
}

std::size_t SessionManager::GetSessionCount() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_sessions.size();
}

} // namespace gs