/**
 * @file session_manager.h
 * @brief Session lifecycle management for netaccess server.
 *
 * Enforces max concurrent sessions and provides session enumeration.
 * Individual session creation/verification is in authenticator.
 */

#pragma once

#include <server/db_layer.h>

namespace session
{

/**
 * @brief Manages active sessions (limits, cleanup, enumeration).
 */
class SessionManager
{
public:
    explicit SessionManager(db::DbLayer& db, const server::ServerConfig& cfg);

    /// Returns true if the server has not reached max concurrent sessions.
    bool canAcceptSession() const;

    /// Revokes all sessions for a given user (force logout everywhere).
    bool forceLogoutAll(qint64 userId);

    /// Revokes a specific session by ID (admin action).
    bool revokeSession(qint64 sessionId);

    /// Returns the number of currently active sessions.
    int activeSessionCount() const;

private:
    db::DbLayer& m_db;
    server::ServerConfig m_cfg;
};

} // namespace session
