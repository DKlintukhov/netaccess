#include <server/pep.h>

#include <QJsonArray>
#include <QJsonDocument>

namespace pep
{

PEP::PEP(pip::PIP& pip, auth::Authenticator& auth, session::SessionManager& sessions, pdp::PDP& pdp)
    : m_pip(pip), m_auth(auth), m_sessions(sessions), m_pdp(pdp)
{
}

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

std::optional<qint64> PEP::validateToken(const QString& token)
{
    auto attrs = m_auth.verifyToken(token);
    if (!attrs)
    {
        return std::nullopt;
    }
    return attrs->user_id;
}

protocol::Response PEP::denied(protocol::Op op, int reqId, protocol::ResultCode code, const QString& message)
{
    protocol::Response resp;
    resp.op = op;
    resp.req_id = reqId;
    resp.status = protocol::Status::denied;
    resp.code = code;
    resp.message = message;
    return resp;
}

protocol::Response PEP::error(protocol::Op op, int reqId, protocol::ResultCode code, const QString& message)
{
    protocol::Response resp;
    resp.op = op;
    resp.req_id = reqId;
    resp.status = protocol::Status::error;
    resp.code = code;
    resp.message = message;
    return resp;
}

protocol::Response PEP::ok(protocol::Op op, int reqId, const QJsonObject& data, const QString& message)
{
    protocol::Response resp;
    resp.op = op;
    resp.req_id = reqId;
    resp.status = protocol::Status::ok;
    resp.code = protocol::ResultCode::OK;
    resp.data = data;
    resp.message = message;
    return resp;
}

bool PEP::checkAccess(qint64 userId, qint64 resourceId, const QString& action, const QString& resourceType)
{
    auto decision = m_pdp.evaluate(userId, resourceId, action, resourceType);
    return decision.allowed;
}

// ---------------------------------------------------------------------------
// Dispatch
// ---------------------------------------------------------------------------

protocol::Response PEP::handle(const protocol::Request& req)
{
    // AUTH is the only operation that doesn't require a token.
    if (req.op == protocol::Op::AUTH)
    {
        return handleAuth(req);
    }

    // All other operations require a valid token.
    auto userId = validateToken(req.token);
    if (!userId)
    {
        return error(req.op, req.req_id, protocol::ResultCode::TOKEN_INVALID,
                     QStringLiteral("Invalid or expired token"));
    }

    switch (req.op)
    {
    case protocol::Op::AUTH:
        Q_UNREACHABLE();
    case protocol::Op::LOGOUT:
        return handleLogout(req);
    case protocol::Op::ME:
        return handleMe(req);
    case protocol::Op::RESOURCE_LIST:
        return handleResourceList(req);
    case protocol::Op::RESOURCE_GET:
        return handleResourceGet(req);
    case protocol::Op::RESOURCE_CREATE:
        return handleResourceCreate(req);
    case protocol::Op::RESOURCE_UPDATE:
        return handleResourceUpdate(req);
    case protocol::Op::RESOURCE_DELETE:
        return handleResourceDelete(req);
    case protocol::Op::POLICY_LIST:
        return handlePolicyList(req);
    case protocol::Op::POLICY_CREATE:
        return handlePolicyCreate(req);
    case protocol::Op::POLICY_UPDATE:
        return handlePolicyUpdate(req);
    case protocol::Op::POLICY_DELETE:
        return handlePolicyDelete(req);
    case protocol::Op::USER_LIST:
        return handleUserList(req);
    case protocol::Op::USER_CREATE:
        return handleUserCreate(req);
    case protocol::Op::USER_UPDATE:
        return handleUserUpdate(req);
    case protocol::Op::USER_DELETE:
        return handleUserDelete(req);
    case protocol::Op::ACCESS_CHECK:
        return handleAccessCheck(req);
    case protocol::Op::GRANT_ACCESS:
        return handleGrantAccess(req);
    case protocol::Op::REVOKE_ACCESS:
        return handleRevokeAccess(req);
    case protocol::Op::AUDIT_QUERY:
        return handleAuditQuery(req);
    }

    return error(req.op, req.req_id, protocol::ResultCode::UNSUPPORTED_OP, QStringLiteral("Unknown operation"));
}

// ---------------------------------------------------------------------------
// AUTH
// ---------------------------------------------------------------------------

protocol::Response PEP::handleAuth(const protocol::Request& req)
{
    const QString username = req.data["username"].toString();
    const QString password = req.data["password"].toString();

    if (username.isEmpty() || password.isEmpty())
    {
        return error(req.op, req.req_id, protocol::ResultCode::VALIDATION_ERROR,
                     QStringLiteral("Missing username or password"));
    }

    auto result = m_auth.authenticate(username, password);

    if (!result.success)
    {
        auto code = protocol::resultCodeFromString(result.error_code);
        return error(req.op, req.req_id, code.value_or(protocol::ResultCode::AUTH_DENIED), result.error_code);
    }

    QJsonObject userData;
    userData["id"] = result.user_id;
    userData["username"] = result.username;
    userData["full_name"] = result.full_name;
    userData["role"] = result.role;
    userData["clearance_level"] = result.clearance_level;
    userData["department"] = result.department;

    QJsonObject data;
    data["token"] = result.token;
    data["user"] = userData;

    m_pip.writeAuditLog(result.user_id, result.username, QStringLiteral("AUTH_SUCCESS"), QStringLiteral("user"),
                        result.user_id, QStringLiteral("ok"));

    return ok(req.op, req.req_id, data);
}

// ---------------------------------------------------------------------------
// LOGOUT
// ---------------------------------------------------------------------------

protocol::Response PEP::handleLogout(const protocol::Request& req)
{
    m_auth.logout(req.token);
    return ok(req.op, req.req_id);
}

// ---------------------------------------------------------------------------
// ME
// ---------------------------------------------------------------------------

protocol::Response PEP::handleMe(const protocol::Request& req)
{
    auto userId = validateToken(req.token);
    if (!userId)
    {
        return error(req.op, req.req_id, protocol::ResultCode::TOKEN_INVALID, QStringLiteral("Invalid token"));
    }

    auto user = m_pip.findUserById(*userId);
    if (!user)
    {
        return error(req.op, req.req_id, protocol::ResultCode::INTERNAL_ERROR, QStringLiteral("User not found"));
    }

    auto attrs = m_pip.getSubjectAttrs(*userId);

    QJsonObject data;
    data["id"] = user->id;
    data["username"] = user->username;
    data["full_name"] = user->full_name;
    data["position"] = user->position;
    data["is_active"] = user->is_active;
    data["role"] = attrs ? attrs->role : QStringLiteral("user");
    data["clearance_level"] = attrs ? attrs->clearance_level : 0;
    data["department"] = attrs ? attrs->department : QString();

    return ok(req.op, req.req_id, data);
}

// ---------------------------------------------------------------------------
// RESOURCE_LIST
// ---------------------------------------------------------------------------

protocol::Response PEP::handleResourceList(const protocol::Request& req)
{
    auto userId = validateToken(req.token);
    if (!userId)
    {
        return error(req.op, req.req_id, protocol::ResultCode::TOKEN_INVALID, QStringLiteral("Invalid token"));
    }

    auto attrs = m_pip.getSubjectAttrs(*userId);
    if (!attrs)
    {
        return error(req.op, req.req_id, protocol::ResultCode::INTERNAL_ERROR, QStringLiteral("No subject attributes"));
    }

    // Admin and auditor see all resources; user sees only accessible ones.
    const QString typeFilter = req.data["resource_type"].toString();
    const int page = req.data["page"].toInt(1);
    const int pageSize = req.data["page_size"].toInt(50);

    auto resources = m_pip.listResources(typeFilter, page, pageSize);
    int total = m_pip.countResources(typeFilter);

    QJsonArray items;
    for (const auto& r : resources)
    {
        // For regular users, check if they have at least read access.
        if (attrs->role == QStringLiteral("user"))
        {
            auto decision = m_pdp.evaluate(*userId, r.id, QStringLiteral("read"), r.resource_type);
            if (!decision.allowed)
            {
                continue;
            }
        }

        QJsonObject obj;
        obj["id"] = r.id;
        obj["name"] = r.name;
        obj["description"] = r.description;
        obj["resource_type"] = r.resource_type;
        obj["address"] = r.address;
        obj["owner_id"] = r.owner_id;
        obj["is_active"] = r.is_active;
        items.append(obj);
    }

    QJsonObject data;
    data["items"] = items;
    data["total"] = total;

    return ok(req.op, req.req_id, data);
}

// ---------------------------------------------------------------------------
// RESOURCE_GET
// ---------------------------------------------------------------------------

protocol::Response PEP::handleResourceGet(const protocol::Request& req)
{
    auto userId = validateToken(req.token);
    if (!userId)
    {
        return error(req.op, req.req_id, protocol::ResultCode::TOKEN_INVALID, QStringLiteral("Invalid token"));
    }

    const qint64 resourceId = req.data["resource_id"].toVariant().toLongLong();
    if (resourceId <= 0)
    {
        return error(req.op, req.req_id, protocol::ResultCode::VALIDATION_ERROR, QStringLiteral("Missing resource_id"));
    }

    auto resource = m_pip.getResource(resourceId);
    if (!resource)
    {
        return denied(req.op, req.req_id, protocol::ResultCode::RESOURCE_NOT_FOUND,
                      QStringLiteral("Resource not found"));
    }

    QJsonObject data;
    data["id"] = resource->id;
    data["name"] = resource->name;
    data["description"] = resource->description;
    data["resource_type"] = resource->resource_type;
    data["address"] = resource->address;
    data["owner_id"] = resource->owner_id;
    data["is_active"] = resource->is_active;

    return ok(req.op, req.req_id, data);
}

// ---------------------------------------------------------------------------
// RESOURCE_CREATE (admin only)
// ---------------------------------------------------------------------------

protocol::Response PEP::handleResourceCreate(const protocol::Request& req)
{
    auto userId = validateToken(req.token);
    if (!userId)
    {
        return error(req.op, req.req_id, protocol::ResultCode::TOKEN_INVALID, QStringLiteral("Invalid token"));
    }

    auto attrs = m_pip.getSubjectAttrs(*userId);
    if (!attrs || attrs->role != QStringLiteral("admin"))
    {
        return denied(req.op, req.req_id, protocol::ResultCode::ACCESS_DENIED, QStringLiteral("Admin role required"));
    }

    const QString name = req.data["name"].toString();
    const QString description = req.data["description"].toString();
    const QString resourceType = req.data["resource_type"].toString();
    const QString address = req.data["address"].toString();

    if (name.isEmpty() || resourceType.isEmpty())
    {
        return error(req.op, req.req_id, protocol::ResultCode::VALIDATION_ERROR,
                     QStringLiteral("Missing name or resource_type"));
    }

    qint64 id = m_pip.createResource(name, description, resourceType, address, *userId);
    if (id < 0)
    {
        return error(req.op, req.req_id, protocol::ResultCode::INTERNAL_ERROR,
                     QStringLiteral("Failed to create resource"));
    }

    m_pip.writeAuditLog(*userId, attrs->role, QStringLiteral("RESOURCE_CREATE"), QStringLiteral("resource"), id,
                        QStringLiteral("ok"));

    QJsonObject data;
    data["id"] = id;
    return ok(req.op, req.req_id, data);
}

// ---------------------------------------------------------------------------
// RESOURCE_UPDATE (admin only)
// ---------------------------------------------------------------------------

protocol::Response PEP::handleResourceUpdate(const protocol::Request& req)
{
    auto userId = validateToken(req.token);
    if (!userId)
    {
        return error(req.op, req.req_id, protocol::ResultCode::TOKEN_INVALID, QStringLiteral("Invalid token"));
    }

    auto attrs = m_pip.getSubjectAttrs(*userId);
    if (!attrs || attrs->role != QStringLiteral("admin"))
    {
        return denied(req.op, req.req_id, protocol::ResultCode::ACCESS_DENIED, QStringLiteral("Admin role required"));
    }

    const qint64 resourceId = req.data["resource_id"].toVariant().toLongLong();
    if (resourceId <= 0)
    {
        return error(req.op, req.req_id, protocol::ResultCode::VALIDATION_ERROR, QStringLiteral("Missing resource_id"));
    }

    auto resource = m_pip.getResource(resourceId);
    if (!resource)
    {
        return denied(req.op, req.req_id, protocol::ResultCode::RESOURCE_NOT_FOUND,
                      QStringLiteral("Resource not found"));
    }

    const QString name = req.data["name"].toString(resource->name);
    const QString description = req.data["description"].toString(resource->description);
    const QString resourceType = req.data["resource_type"].toString(resource->resource_type);
    const QString address = req.data["address"].toString(resource->address);
    const bool isActive = req.data["is_active"].toBool(resource->is_active);

    if (!m_pip.updateResource(resourceId, name, description, resourceType, address, isActive))
    {
        return error(req.op, req.req_id, protocol::ResultCode::INTERNAL_ERROR,
                     QStringLiteral("Failed to update resource"));
    }

    m_pip.writeAuditLog(*userId, attrs->role, QStringLiteral("RESOURCE_UPDATE"), QStringLiteral("resource"), resourceId,
                        QStringLiteral("ok"));

    return ok(req.op, req.req_id);
}

// ---------------------------------------------------------------------------
// RESOURCE_DELETE (admin only)
// ---------------------------------------------------------------------------

protocol::Response PEP::handleResourceDelete(const protocol::Request& req)
{
    auto userId = validateToken(req.token);
    if (!userId)
    {
        return error(req.op, req.req_id, protocol::ResultCode::TOKEN_INVALID, QStringLiteral("Invalid token"));
    }

    auto attrs = m_pip.getSubjectAttrs(*userId);
    if (!attrs || attrs->role != QStringLiteral("admin"))
    {
        return denied(req.op, req.req_id, protocol::ResultCode::ACCESS_DENIED, QStringLiteral("Admin role required"));
    }

    const qint64 resourceId = req.data["resource_id"].toVariant().toLongLong();
    if (resourceId <= 0)
    {
        return error(req.op, req.req_id, protocol::ResultCode::VALIDATION_ERROR, QStringLiteral("Missing resource_id"));
    }

    if (!m_pip.deleteResource(resourceId))
    {
        return error(req.op, req.req_id, protocol::ResultCode::INTERNAL_ERROR,
                     QStringLiteral("Failed to delete resource"));
    }

    m_pip.writeAuditLog(*userId, attrs->role, QStringLiteral("RESOURCE_DELETE"), QStringLiteral("resource"), resourceId,
                        QStringLiteral("ok"));

    return ok(req.op, req.req_id);
}

// ---------------------------------------------------------------------------
// POLICY_LIST (admin, auditor)
// ---------------------------------------------------------------------------

protocol::Response PEP::handlePolicyList(const protocol::Request& req)
{
    auto userId = validateToken(req.token);
    if (!userId)
    {
        return error(req.op, req.req_id, protocol::ResultCode::TOKEN_INVALID, QStringLiteral("Invalid token"));
    }

    auto attrs = m_pip.getSubjectAttrs(*userId);
    if (!attrs || (attrs->role != QStringLiteral("admin") && attrs->role != QStringLiteral("auditor")))
    {
        return denied(req.op, req.req_id, protocol::ResultCode::ACCESS_DENIED,
                      QStringLiteral("Admin or auditor role required"));
    }

    const int page = req.data["page"].toInt(1);
    const int pageSize = req.data["page_size"].toInt(50);

    auto policies = m_pdp.listPolicies(page, pageSize);
    int total = m_pdp.countPolicies();

    QJsonArray items;
    for (const auto& p : policies)
    {
        QJsonObject obj;
        obj["id"] = p.id;
        obj["name"] = p.name;
        obj["enabled"] = p.enabled;
        obj["action"] = p.action;
        obj["role_required"] = p.role_required;
        obj["department_required"] = p.department_required;
        obj["min_clearance"] = p.min_clearance < 0 ? QJsonValue() : p.min_clearance;
        obj["resource_type"] = p.resource_type;
        obj["subject_id"] = p.subject_id <= 0 ? QJsonValue() : static_cast<qint64>(p.subject_id);
        obj["resource_id"] = p.resource_id <= 0 ? QJsonValue() : static_cast<qint64>(p.resource_id);
        obj["priority"] = p.priority;
        items.append(obj);
    }

    QJsonObject data;
    data["items"] = items;
    data["total"] = total;

    return ok(req.op, req.req_id, data);
}

// ---------------------------------------------------------------------------
// POLICY_CREATE (admin)
// ---------------------------------------------------------------------------

protocol::Response PEP::handlePolicyCreate(const protocol::Request& req)
{
    auto userId = validateToken(req.token);
    if (!userId)
    {
        return error(req.op, req.req_id, protocol::ResultCode::TOKEN_INVALID, QStringLiteral("Invalid token"));
    }

    auto attrs = m_pip.getSubjectAttrs(*userId);
    if (!attrs || attrs->role != QStringLiteral("admin"))
    {
        return denied(req.op, req.req_id, protocol::ResultCode::ACCESS_DENIED, QStringLiteral("Admin role required"));
    }

    const QString name = req.data["name"].toString();
    const QString action = req.data["action"].toString();
    if (name.isEmpty() || action.isEmpty())
    {
        return error(req.op, req.req_id, protocol::ResultCode::VALIDATION_ERROR,
                     QStringLiteral("Missing name or action"));
    }

    pip::PolicyRecord p;
    p.name = name;
    p.action = action;
    p.enabled = req.data["enabled"].toBool(true);
    p.priority = req.data["priority"].toInt(0);
    p.role_required = req.data["role_required"].toString();
    p.department_required = req.data["department_required"].toString();
    p.min_clearance = req.data["min_clearance"].toInt(-1);
    p.resource_type = req.data["resource_type"].toString();
    p.subject_id = req.data["subject_id"].toVariant().toLongLong(0);
    p.resource_id = req.data["resource_id"].toVariant().toLongLong(0);

    qint64 id = m_pdp.createPolicy(p, *userId);
    if (id < 0)
    {
        return error(req.op, req.req_id, protocol::ResultCode::INTERNAL_ERROR,
                     QStringLiteral("Failed to create policy"));
    }

    m_pip.writeAuditLog(*userId, attrs->role, QStringLiteral("POLICY_CREATE"), QStringLiteral("policy"), id,
                        QStringLiteral("ok"));

    QJsonObject data;
    data["id"] = id;
    return ok(req.op, req.req_id, data);
}

// ---------------------------------------------------------------------------
// POLICY_UPDATE (admin)
// ---------------------------------------------------------------------------

protocol::Response PEP::handlePolicyUpdate(const protocol::Request& req)
{
    auto userId = validateToken(req.token);
    if (!userId)
    {
        return error(req.op, req.req_id, protocol::ResultCode::TOKEN_INVALID, QStringLiteral("Invalid token"));
    }

    auto attrs = m_pip.getSubjectAttrs(*userId);
    if (!attrs || attrs->role != QStringLiteral("admin"))
    {
        return denied(req.op, req.req_id, protocol::ResultCode::ACCESS_DENIED, QStringLiteral("Admin role required"));
    }

    const qint64 policyId = req.data["policy_id"].toVariant().toLongLong();
    if (policyId <= 0)
    {
        return error(req.op, req.req_id, protocol::ResultCode::VALIDATION_ERROR, QStringLiteral("Missing policy_id"));
    }

    auto existing = m_pdp.listPolicies(1, 999999);
    const pip::PolicyRecord* found = nullptr;
    for (const auto& p : existing)
    {
        if (p.id == policyId)
        {
            found = &p;
            break;
        }
    }
    if (!found)
    {
        return denied(req.op, req.req_id, protocol::ResultCode::RESOURCE_NOT_FOUND, QStringLiteral("Policy not found"));
    }

    pip::PolicyRecord p = *found;
    if (req.data.contains("name"))
        p.name = req.data["name"].toString();
    if (req.data.contains("action"))
        p.action = req.data["action"].toString();
    if (req.data.contains("enabled"))
        p.enabled = req.data["enabled"].toBool();
    if (req.data.contains("priority"))
        p.priority = req.data["priority"].toInt();
    if (req.data.contains("role_required"))
        p.role_required = req.data["role_required"].toString();
    if (req.data.contains("department_required"))
        p.department_required = req.data["department_required"].toString();
    if (req.data.contains("min_clearance"))
        p.min_clearance = req.data["min_clearance"].toInt(-1);
    if (req.data.contains("resource_type"))
        p.resource_type = req.data["resource_type"].toString();
    if (req.data.contains("subject_id"))
        p.subject_id = req.data["subject_id"].toVariant().toLongLong(0);
    if (req.data.contains("resource_id"))
        p.resource_id = req.data["resource_id"].toVariant().toLongLong(0);

    if (!m_pdp.updatePolicy(policyId, p))
    {
        return error(req.op, req.req_id, protocol::ResultCode::INTERNAL_ERROR,
                     QStringLiteral("Failed to update policy"));
    }

    m_pip.writeAuditLog(*userId, attrs->role, QStringLiteral("POLICY_UPDATE"), QStringLiteral("policy"), policyId,
                        QStringLiteral("ok"));

    return ok(req.op, req.req_id);
}

// ---------------------------------------------------------------------------
// POLICY_DELETE (admin)
// ---------------------------------------------------------------------------

protocol::Response PEP::handlePolicyDelete(const protocol::Request& req)
{
    auto userId = validateToken(req.token);
    if (!userId)
    {
        return error(req.op, req.req_id, protocol::ResultCode::TOKEN_INVALID, QStringLiteral("Invalid token"));
    }

    auto attrs = m_pip.getSubjectAttrs(*userId);
    if (!attrs || attrs->role != QStringLiteral("admin"))
    {
        return denied(req.op, req.req_id, protocol::ResultCode::ACCESS_DENIED, QStringLiteral("Admin role required"));
    }

    const qint64 policyId = req.data["policy_id"].toVariant().toLongLong();
    if (policyId <= 0)
    {
        return error(req.op, req.req_id, protocol::ResultCode::VALIDATION_ERROR, QStringLiteral("Missing policy_id"));
    }

    if (!m_pdp.deletePolicy(policyId))
    {
        return error(req.op, req.req_id, protocol::ResultCode::INTERNAL_ERROR,
                     QStringLiteral("Failed to delete policy"));
    }

    m_pip.writeAuditLog(*userId, attrs->role, QStringLiteral("POLICY_DELETE"), QStringLiteral("policy"), policyId,
                        QStringLiteral("ok"));

    return ok(req.op, req.req_id);
}

// ---------------------------------------------------------------------------
// USER_LIST (admin)
// ---------------------------------------------------------------------------

protocol::Response PEP::handleUserList(const protocol::Request& req)
{
    auto userId = validateToken(req.token);
    if (!userId)
    {
        return error(req.op, req.req_id, protocol::ResultCode::TOKEN_INVALID, QStringLiteral("Invalid token"));
    }

    auto attrs = m_pip.getSubjectAttrs(*userId);
    if (!attrs || attrs->role != QStringLiteral("admin"))
    {
        return denied(req.op, req.req_id, protocol::ResultCode::ACCESS_DENIED, QStringLiteral("Admin role required"));
    }

    const int page = req.data["page"].toInt(1);
    const int pageSize = req.data["page_size"].toInt(50);

    auto users = m_pip.listUsers(page, pageSize);
    int total = m_pip.countUsers();

    QJsonArray items;
    for (const auto& u : users)
    {
        auto sa = m_pip.getSubjectAttrs(u.id);
        QJsonObject obj;
        obj["id"] = u.id;
        obj["username"] = u.username;
        obj["full_name"] = u.full_name;
        obj["position"] = u.position;
        obj["is_active"] = u.is_active;
        obj["role"] = sa ? sa->role : QStringLiteral("user");
        obj["clearance_level"] = sa ? sa->clearance_level : 0;
        obj["department"] = sa ? sa->department : QString();
        items.append(obj);
    }

    QJsonObject data;
    data["items"] = items;
    data["total"] = total;

    return ok(req.op, req.req_id, data);
}

// ---------------------------------------------------------------------------
// USER_CREATE (admin)
// ---------------------------------------------------------------------------

protocol::Response PEP::handleUserCreate(const protocol::Request& req)
{
    auto userId = validateToken(req.token);
    if (!userId)
    {
        return error(req.op, req.req_id, protocol::ResultCode::TOKEN_INVALID, QStringLiteral("Invalid token"));
    }

    auto attrs = m_pip.getSubjectAttrs(*userId);
    if (!attrs || attrs->role != QStringLiteral("admin"))
    {
        return denied(req.op, req.req_id, protocol::ResultCode::ACCESS_DENIED, QStringLiteral("Admin role required"));
    }

    const QString username = req.data["username"].toString();
    const QString password = req.data["password"].toString();
    const QString fullName = req.data["full_name"].toString();
    if (username.isEmpty() || password.isEmpty() || fullName.isEmpty())
    {
        return error(req.op, req.req_id, protocol::ResultCode::VALIDATION_ERROR,
                     QStringLiteral("Missing username, password, or full_name"));
    }

    // Hash password.
    const QString hashResult = auth::Authenticator::hashPassword(password);
    const QStringList parts = hashResult.split(':');
    const QString passwordHash = parts.value(0);
    const QString salt = parts.value(1);

    const QString role = req.data["role"].toString(QStringLiteral("user"));
    const int clearanceLevel = req.data["clearance_level"].toInt(0);
    const QString department = req.data["department"].toString();
    const QString position = req.data["position"].toString();

    qint64 newUserId =
        m_pip.createUser(username, passwordHash, salt, fullName, position, role, clearanceLevel, department);
    if (newUserId < 0)
    {
        return error(req.op, req.req_id, protocol::ResultCode::INTERNAL_ERROR, QStringLiteral("Failed to create user"));
    }

    m_pip.writeAuditLog(*userId, attrs->role, QStringLiteral("USER_CREATE"), QStringLiteral("user"), newUserId,
                        QStringLiteral("ok"));

    QJsonObject data;
    data["id"] = newUserId;
    return ok(req.op, req.req_id, data);
}

// ---------------------------------------------------------------------------
// USER_UPDATE (admin)
// ---------------------------------------------------------------------------

protocol::Response PEP::handleUserUpdate(const protocol::Request& req)
{
    auto userId = validateToken(req.token);
    if (!userId)
    {
        return error(req.op, req.req_id, protocol::ResultCode::TOKEN_INVALID, QStringLiteral("Invalid token"));
    }

    auto attrs = m_pip.getSubjectAttrs(*userId);
    if (!attrs || attrs->role != QStringLiteral("admin"))
    {
        return denied(req.op, req.req_id, protocol::ResultCode::ACCESS_DENIED, QStringLiteral("Admin role required"));
    }

    const qint64 targetUserId = req.data["user_id"].toVariant().toLongLong();
    if (targetUserId <= 0)
    {
        return error(req.op, req.req_id, protocol::ResultCode::VALIDATION_ERROR, QStringLiteral("Missing user_id"));
    }

    auto target = m_pip.findUserById(targetUserId);
    if (!target)
    {
        return denied(req.op, req.req_id, protocol::ResultCode::RESOURCE_NOT_FOUND, QStringLiteral("User not found"));
    }

    const QString fullName = req.data["full_name"].toString(target->full_name);
    const QString position = req.data["position"].toString(target->position);
    const bool isActive = req.data["is_active"].toBool(target->is_active);

    if (!m_pip.updateUser(targetUserId, fullName, position, isActive))
    {
        return error(req.op, req.req_id, protocol::ResultCode::INTERNAL_ERROR, QStringLiteral("Failed to update user"));
    }

    // Update subject attrs if provided.
    if (req.data.contains("role") || req.data.contains("clearance_level") || req.data.contains("department"))
    {
        auto sa = m_pip.getSubjectAttrs(targetUserId);
        if (sa)
        {
            const QString newRole = req.data["role"].toString(sa->role);
            const int newClearance = req.data["clearance_level"].toInt(sa->clearance_level);
            const QString newDept = req.data["department"].toString(sa->department);
            m_pip.updateSubjectAttrs(targetUserId, newRole, newClearance, newDept);
        }
    }

    m_pip.writeAuditLog(*userId, attrs->role, QStringLiteral("USER_UPDATE"), QStringLiteral("user"), targetUserId,
                        QStringLiteral("ok"));

    return ok(req.op, req.req_id);
}

// ---------------------------------------------------------------------------
// USER_DELETE (admin)
// ---------------------------------------------------------------------------

protocol::Response PEP::handleUserDelete(const protocol::Request& req)
{
    auto userId = validateToken(req.token);
    if (!userId)
    {
        return error(req.op, req.req_id, protocol::ResultCode::TOKEN_INVALID, QStringLiteral("Invalid token"));
    }

    auto attrs = m_pip.getSubjectAttrs(*userId);
    if (!attrs || attrs->role != QStringLiteral("admin"))
    {
        return denied(req.op, req.req_id, protocol::ResultCode::ACCESS_DENIED, QStringLiteral("Admin role required"));
    }

    const qint64 targetUserId = req.data["user_id"].toVariant().toLongLong();
    if (targetUserId <= 0)
    {
        return error(req.op, req.req_id, protocol::ResultCode::VALIDATION_ERROR, QStringLiteral("Missing user_id"));
    }

    if (targetUserId == *userId)
    {
        return error(req.op, req.req_id, protocol::ResultCode::VALIDATION_ERROR,
                     QStringLiteral("Cannot delete yourself"));
    }

    if (!m_pip.deleteUser(targetUserId))
    {
        return error(req.op, req.req_id, protocol::ResultCode::INTERNAL_ERROR, QStringLiteral("Failed to delete user"));
    }

    m_pip.writeAuditLog(*userId, attrs->role, QStringLiteral("USER_DELETE"), QStringLiteral("user"), targetUserId,
                        QStringLiteral("ok"));

    return ok(req.op, req.req_id);
}

// ---------------------------------------------------------------------------
// ACCESS_CHECK
// ---------------------------------------------------------------------------

protocol::Response PEP::handleAccessCheck(const protocol::Request& req)
{
    auto userId = validateToken(req.token);
    if (!userId)
    {
        return error(req.op, req.req_id, protocol::ResultCode::TOKEN_INVALID, QStringLiteral("Invalid token"));
    }

    const qint64 resourceId = req.data["resource_id"].toVariant().toLongLong();
    const QString action = req.data["action"].toString();

    if (resourceId <= 0 || action.isEmpty())
    {
        return error(req.op, req.req_id, protocol::ResultCode::VALIDATION_ERROR,
                     QStringLiteral("Missing resource_id or action"));
    }

    auto resource = m_pip.getResource(resourceId);
    if (!resource)
    {
        return denied(req.op, req.req_id, protocol::ResultCode::RESOURCE_NOT_FOUND,
                      QStringLiteral("Resource not found"));
    }

    auto decision = m_pdp.evaluate(*userId, resourceId, action, resource->resource_type);

    if (decision.allowed)
    {
        return ok(req.op, req.req_id);
    }

    return denied(req.op, req.req_id, protocol::ResultCode::ACCESS_DENIED, decision.reason);
}

// ---------------------------------------------------------------------------
// GRANT_ACCESS (admin)
// ---------------------------------------------------------------------------

protocol::Response PEP::handleGrantAccess(const protocol::Request& req)
{
    auto userId = validateToken(req.token);
    if (!userId)
    {
        return error(req.op, req.req_id, protocol::ResultCode::TOKEN_INVALID, QStringLiteral("Invalid token"));
    }

    auto attrs = m_pip.getSubjectAttrs(*userId);
    if (!attrs || attrs->role != QStringLiteral("admin"))
    {
        return denied(req.op, req.req_id, protocol::ResultCode::ACCESS_DENIED, QStringLiteral("Admin role required"));
    }

    const qint64 subjectId = req.data["subject_id"].toVariant().toLongLong();
    const qint64 resourceId = req.data["resource_id"].toVariant().toLongLong();
    const QString action = req.data["action"].toString();

    if (subjectId <= 0 || resourceId <= 0 || action.isEmpty())
    {
        return error(req.op, req.req_id, protocol::ResultCode::VALIDATION_ERROR,
                     QStringLiteral("Missing subject_id, resource_id, or action"));
    }

    // Verify subject and resource exist.
    auto subject = m_pip.findUserById(subjectId);
    if (!subject)
    {
        return denied(req.op, req.req_id, protocol::ResultCode::RESOURCE_NOT_FOUND,
                      QStringLiteral("Subject user not found"));
    }

    auto resource = m_pip.getResource(resourceId);
    if (!resource)
    {
        return denied(req.op, req.req_id, protocol::ResultCode::RESOURCE_NOT_FOUND,
                      QStringLiteral("Resource not found"));
    }

    // Create a specific policy for this grant.
    pip::PolicyRecord p;
    p.name = QStringLiteral("grant-%1-%2-%3").arg(subjectId).arg(resourceId).arg(action);
    p.action = action;
    p.enabled = true;
    p.priority = 10; // Higher than base policies (1).
    p.subject_id = subjectId;
    p.resource_id = resourceId;

    qint64 policyId = m_pdp.createPolicy(p, *userId);
    if (policyId < 0)
    {
        return error(req.op, req.req_id, protocol::ResultCode::INTERNAL_ERROR,
                     QStringLiteral("Failed to create grant policy"));
    }

    m_pip.writeAuditLog(
        *userId, attrs->role, QStringLiteral("POLICY_CREATE"), QStringLiteral("policy"), policyId, QStringLiteral("ok"),
        QStringLiteral("{\"grant\":true,\"subject\":%1,\"resource\":%2}").arg(subjectId).arg(resourceId));

    QJsonObject data;
    data["policy_id"] = policyId;
    return ok(req.op, req.req_id, data);
}

// ---------------------------------------------------------------------------
// REVOKE_ACCESS (admin)
// ---------------------------------------------------------------------------

protocol::Response PEP::handleRevokeAccess(const protocol::Request& req)
{
    auto userId = validateToken(req.token);
    if (!userId)
    {
        return error(req.op, req.req_id, protocol::ResultCode::TOKEN_INVALID, QStringLiteral("Invalid token"));
    }

    auto attrs = m_pip.getSubjectAttrs(*userId);
    if (!attrs || attrs->role != QStringLiteral("admin"))
    {
        return denied(req.op, req.req_id, protocol::ResultCode::ACCESS_DENIED, QStringLiteral("Admin role required"));
    }

    const qint64 policyId = req.data["policy_id"].toVariant().toLongLong();
    if (policyId <= 0)
    {
        return error(req.op, req.req_id, protocol::ResultCode::VALIDATION_ERROR, QStringLiteral("Missing policy_id"));
    }

    if (!m_pdp.deletePolicy(policyId))
    {
        return error(req.op, req.req_id, protocol::ResultCode::INTERNAL_ERROR,
                     QStringLiteral("Failed to revoke access"));
    }

    m_pip.writeAuditLog(*userId, attrs->role, QStringLiteral("POLICY_DELETE"), QStringLiteral("policy"), policyId,
                        QStringLiteral("ok"));

    return ok(req.op, req.req_id);
}

// ---------------------------------------------------------------------------
// AUDIT_QUERY (admin, auditor)
// ---------------------------------------------------------------------------

protocol::Response PEP::handleAuditQuery(const protocol::Request& req)
{
    auto userId = validateToken(req.token);
    if (!userId)
    {
        return error(req.op, req.req_id, protocol::ResultCode::TOKEN_INVALID, QStringLiteral("Invalid token"));
    }

    auto attrs = m_pip.getSubjectAttrs(*userId);
    if (!attrs || (attrs->role != QStringLiteral("admin") && attrs->role != QStringLiteral("auditor")))
    {
        return denied(req.op, req.req_id, protocol::ResultCode::ACCESS_DENIED,
                      QStringLiteral("Admin or auditor role required"));
    }

    pip::AuditFilter filter;
    filter.from = req.data["from"].toString();
    filter.to = req.data["to"].toString();
    filter.actor_id = req.data["actor_id"].toVariant().toLongLong(0);
    filter.action = req.data["action"].toString();
    filter.page = req.data["page"].toInt(1);
    filter.page_size = req.data["page_size"].toInt(100);

    auto records = m_pip.queryAuditLog(filter);
    int total = m_pip.countAuditLog(filter);

    QJsonArray items;
    for (const auto& r : records)
    {
        QJsonObject obj;
        obj["id"] = r.id;
        obj["ts"] = r.ts;
        obj["actor_id"] = r.actor_id;
        obj["actor_name"] = r.actor_name;
        obj["action"] = r.action;
        obj["target_type"] = r.target_type;
        obj["target_id"] = r.target_id;
        obj["result"] = r.result;
        if (!r.details.isEmpty())
        {
            obj["details"] = QJsonDocument::fromJson(r.details.toUtf8()).object();
        }
        items.append(obj);
    }

    QJsonObject data;
    data["items"] = items;
    data["total"] = total;

    return ok(req.op, req.req_id, data);
}

} // namespace pep
