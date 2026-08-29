#include <server/session_manager.h>

namespace session
{

SessionManager::SessionManager(pip::PIP& pip, const server::ServerConfig& cfg) : m_pip(pip), m_cfg(cfg)
{
}

bool SessionManager::canAcceptSession() const
{
    return m_pip.countActiveSessions() < m_cfg.max_sessions;
}

bool SessionManager::forceLogoutAll(qint64 userId)
{
    return m_pip.revokeAllSessions(userId);
}

bool SessionManager::revokeSession(qint64 sessionId)
{
    return m_pip.revokeSession(sessionId);
}

int SessionManager::activeSessionCount() const
{
    return m_pip.countActiveSessions();
}

} // namespace session
