#pragma once

#include "common/Types.h"
#include "net/Session.h"

#include <boost/asio.hpp>
#include <map>
#include <memory>
#include <mutex>

namespace gs
{

class SessionManager
{
public:
    explicit SessionManager(boost::asio::io_context& in_io);
    ~SessionManager();

    SessionManager(const SessionManager&) = delete;
    SessionManager& operator=(const SessionManager&) = delete;

    std::shared_ptr<Session> CreateSession();
    void RemoveSession(SessionId in_session_id);
    std::shared_ptr<Session> GetSession(SessionId in_session_id);

    std::size_t GetSessionCount() const;

private:
    boost::asio::io_context&                    m_io;
    std::map<SessionId, std::shared_ptr<Session>> m_sessions;
    SessionId                                   m_next_session_id;
    mutable std::mutex                          m_mutex;
};

} // namespace gs