#include <server/session_manager.h>

namespace session
{

SessionManager::SessionManager(db::DbLayer& db, const server::ServerConfig& cfg) : m_db(db), m_cfg(cfg)
{
}

bool SessionManager::canAcceptSession() const
{
    return m_db.countActiveSessions() < m_cfg.max_sessions;
}

bool SessionManager::forceLogoutAll(qint64 userId)
{
    return m_db.revokeAllSessions(userId);
}

bool SessionManager::revokeSession(qint64 sessionId)
{
    return m_db.revokeSession(sessionId);
}

int SessionManager::activeSessionCount() const
{
    return m_db.countActiveSessions();
}

} // namespace session
