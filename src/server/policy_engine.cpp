#include <server/policy_engine.h>

#include <QDateTime>

namespace policy
{

PolicyEngine::PolicyEngine(db::DbLayer& db) : m_db(db)
{
}

Decision PolicyEngine::evaluate(qint64 userId, qint64 resourceId, const QString& action, const QString& resourceType)
{
    // 1. Check subject is active.
    auto user = m_db.findUserById(userId);
    if (!user || !user->is_active)
    {
        return {false, QStringLiteral("account_inactive")};
    }

    // 2. Check lockout.
    if (!user->locked_until.isEmpty())
    {
        QDateTime lockedUntil = QDateTime::fromString(user->locked_until, Qt::ISODate);
        if (lockedUntil.isValid() && lockedUntil > QDateTime::currentDateTimeUtc())
        {
            return {false, QStringLiteral("account_locked")};
        }
    }

    // 3. Get subject attributes.
    auto attrs = m_db.getSubjectAttrs(userId);
    if (!attrs)
    {
        return {false, QStringLiteral("no_subject_attrs")};
    }

    // 4. Load enabled policies matching the action (or wildcard '*').
    auto policies = m_db.findEnabledPolicies(action);

    // 5. Find best matching policy by priority.
    const db::PolicyRecord* best = nullptr;
    for (const auto& pol : policies)
    {
        // Check role condition.
        if (!pol.role_required.isEmpty() && pol.role_required != attrs->role)
        {
            continue;
        }

        // Check department condition.
        if (!pol.department_required.isEmpty() && pol.department_required != attrs->department)
        {
            continue;
        }

        // Check clearance level.
        if (pol.min_clearance >= 0 && attrs->clearance_level < pol.min_clearance)
        {
            continue;
        }

        // Check resource type.
        if (!pol.resource_type.isEmpty() && pol.resource_type != resourceType)
        {
            continue;
        }

        // Check specific subject.
        if (pol.subject_id > 0 && static_cast<qint64>(pol.subject_id) != userId)
        {
            continue;
        }

        // Check specific resource.
        if (pol.resource_id > 0 && static_cast<qint64>(pol.resource_id) != resourceId)
        {
            continue;
        }

        // This policy matches.
        if (best == nullptr || pol.priority > best->priority)
        {
            best = &pol;
        }
    }

    if (best == nullptr)
    {
        return {false, QStringLiteral("no_matching_policy")};
    }

    return {true, QStringLiteral("policy:") + best->name};
}

// ---------------------------------------------------------------------------
// Policy management
// ---------------------------------------------------------------------------

QVector<db::PolicyRecord> PolicyEngine::listPolicies(int page, int pageSize)
{
    return m_db.listPolicies(page, pageSize);
}

int PolicyEngine::countPolicies()
{
    return m_db.countPolicies();
}

qint64 PolicyEngine::createPolicy(const db::PolicyRecord& policy, qint64 createdBy)
{
    return m_db.createPolicy(policy.name, policy.action, policy.enabled, policy.priority, policy.role_required,
                             policy.department_required, policy.min_clearance, policy.resource_type, policy.subject_id,
                             policy.resource_id, createdBy);
}

bool PolicyEngine::updatePolicy(qint64 id, const db::PolicyRecord& policy)
{
    return m_db.updatePolicy(id, policy.name, policy.action, policy.enabled, policy.priority, policy.role_required,
                             policy.department_required, policy.min_clearance, policy.resource_type, policy.subject_id,
                             policy.resource_id);
}

bool PolicyEngine::deletePolicy(qint64 id)
{
    return m_db.deletePolicy(id);
}

} // namespace policy
