/**
 * @file session_manager.h
 * @brief Session lifecycle management for netaccess server.
 *
 * Enforces max concurrent sessions and provides session enumeration.
 * Individual session creation/verification is in authenticator.
 */

#pragma once

#include <server/pip.h>

namespace session
{

/**
 * @brief Manages active sessions (limits, cleanup, enumeration).
 */
class SessionManager
{
public:
    explicit SessionManager(pip::PIP& pip, const server::ServerConfig& cfg);

    bool canAcceptSession() const;
    bool forceLogoutAll(qint64 userId);
    bool revokeSession(qint64 sessionId);
    int activeSessionCount() const;

private:
    pip::PIP& m_pip;
    server::ServerConfig m_cfg;
};

} // namespace session
