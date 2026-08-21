/**
 * @file handlers.h
 * @brief Request handlers for all protocol operations.
 *
 * Dispatches incoming requests, validates tokens, applies ABAC checks,
 * and returns structured responses.
 */

#pragma once

#include <netaccess/protocol.h>

#include <server/authenticator.h>
#include <server/db_layer.h>
#include <server/policy_engine.h>
#include <server/session_manager.h>

namespace handlers
{

/**
 * @brief Request handler dispatcher.
 */
class Handler
{
public:
    Handler(db::DbLayer& db, auth::Authenticator& auth, session::SessionManager& sessions,
            policy::PolicyEngine& policy);

    /**
     * @brief Dispatches a request and returns a response.
     */
    protocol::Response handle(const protocol::Request& req);

private:
    // Token validation: extracts user_id from token, returns empty on failure.
    std::optional<qint64> validateToken(const QString& token);

    // Individual handlers.
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

    // Helper: create a denied response.
    protocol::Response denied(protocol::Op op, int reqId, protocol::ResultCode code, const QString& message);

    // Helper: create an error response.
    protocol::Response error(protocol::Op op, int reqId, protocol::ResultCode code, const QString& message);

    // Helper: create an ok response.
    protocol::Response ok(protocol::Op op, int reqId, const QJsonObject& data = {}, const QString& message = {});

    // Helper: check ABAC access.
    bool checkAccess(qint64 userId, qint64 resourceId, const QString& action, const QString& resourceType);

    db::DbLayer& m_db;
    auth::Authenticator& m_auth;
    session::SessionManager& m_sessions;
    policy::PolicyEngine& m_policy;
};

} // namespace handlers
