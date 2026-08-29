#include <server/pdp.h>

#include <QDateTime>

namespace pdp
{

PDP::PDP(pip::PIP& pip) : m_pip(pip)
{
}

Decision PDP::evaluate(qint64 userId, qint64 resourceId, const QString& action, const QString& resourceType)
{
    auto user = m_pip.findUserById(userId);
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
    auto attrs = m_pip.getSubjectAttrs(userId);
    if (!attrs)
    {
        return {false, QStringLiteral("no_subject_attrs")};
    }

    // 4. Load enabled policies matching the action (or wildcard '*').
    auto policies = m_pip.findEnabledPolicies(action);

    // 5. Find best matching policy by priority.
    const pip::PolicyRecord* best = nullptr;
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

QVector<pip::PolicyRecord> PDP::listPolicies(int page, int pageSize)
{
    return m_pip.listPolicies(page, pageSize);
}

int PDP::countPolicies()
{
    return m_pip.countPolicies();
}

qint64 PDP::createPolicy(const pip::PolicyRecord& policy, qint64 createdBy)
{
    return m_pip.createPolicy(policy.name, policy.action, policy.enabled, policy.priority, policy.role_required,
                              policy.department_required, policy.min_clearance, policy.resource_type, policy.subject_id,
                              policy.resource_id, createdBy);
}

bool PDP::updatePolicy(qint64 id, const pip::PolicyRecord& policy)
{
    return m_pip.updatePolicy(id, policy.name, policy.action, policy.enabled, policy.priority, policy.role_required,
                              policy.department_required, policy.min_clearance, policy.resource_type, policy.subject_id,
                              policy.resource_id);
}

bool PDP::deletePolicy(qint64 id)
{
    return m_pip.deletePolicy(id);
}

} // namespace pdp
