/**
 * @file pep.h
 * @brief Policy Enforcement Point (PEP) — enforces access decisions.
 *
 * Dispatches incoming requests, validates tokens, delegates ABAC checks
 * to the PDP, and returns structured responses.  All protected operations
 * must pass through the PEP before reaching resource handlers.
 */

#pragma once

#include <netaccess/protocol.h>

#include <server/authenticator.h>
#include <server/pdp.h>
#include <server/pip.h>
#include <server/session_manager.h>

namespace pep
{

/**
 * @brief Policy Enforcement Point — enforces ABAC decisions.
 */
class PEP
{
public:
    PEP(pip::PIP& pip, auth::Authenticator& auth, session::SessionManager& sessions, pdp::PDP& pdp);

    /**
     * @brief Dispatches a request and returns a response.
     */
    protocol::Response handle(const protocol::Request& req);

private:
    std::optional<qint64> validateToken(const QString& token);

    protocol::Response handleAuth(const protocol::Request& req);
    protocol::Response handleLogout(const protocol::Request& req);
    protocol::Response handleMe(const protocol::Request& req);
    protocol::Response handleResourceList(const protocol::Request& req);
    protocol::Response handleResourceGet(const protocol::Request& req);
    protocol::Response handleResourceCreate(const protocol::Request& req);
    protocol::Response handleResourceUpdate(const protocol::Request& req);
    protocol::Response handleResourceDelete(const protocol::Request& req);
    protocol::Response handlePolicyList(const protocol::Request& req);
    protocol::Response handlePolicyCreate(const protocol::Request& req);
    protocol::Response handlePolicyUpdate(const protocol::Request& req);
    protocol::Response handlePolicyDelete(const protocol::Request& req);
    protocol::Response handleUserList(const protocol::Request& req);
    protocol::Response handleUserCreate(const protocol::Request& req);
    protocol::Response handleUserUpdate(const protocol::Request& req);
    protocol::Response handleUserDelete(const protocol::Request& req);
    protocol::Response handleAccessCheck(const protocol::Request& req);
    protocol::Response handleGrantAccess(const protocol::Request& req);
    protocol::Response handleRevokeAccess(const protocol::Request& req);
    protocol::Response handleAuditQuery(const protocol::Request& req);

    protocol::Response denied(protocol::Op op, int reqId, protocol::ResultCode code, const QString& message);
    protocol::Response error(protocol::Op op, int reqId, protocol::ResultCode code, const QString& message);
    protocol::Response ok(protocol::Op op, int reqId, const QJsonObject& data = {}, const QString& message = {});
    bool checkAccess(qint64 userId, qint64 resourceId, const QString& action, const QString& resourceType);

    pip::PIP& m_pip;
    auth::Authenticator& m_auth;
    session::SessionManager& m_sessions;
    pdp::PDP& m_pdp;
};

} // namespace pep
