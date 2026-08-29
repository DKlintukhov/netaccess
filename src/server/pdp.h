/**
 * @file pdp.h
 * @brief Policy Decision Point (PDP) — evaluates ABAC access requests.
 *
 * Loads policies from the database (via PIP) and evaluates access decisions.
 * Pure ABAC: deny by default.  Deny-overrides on conflict.
 */

#pragma once

#include <QString>
#include <QVector>

#include <server/pip.h>

namespace pdp
{

/**
 * @brief ABAC decision result.
 */
struct Decision
{
    bool allowed = false;
    QString reason;
};

/**
 * @brief Policy Decision Point — evaluates ABAC policies.
 */
class PDP
{
public:
    explicit PDP(pip::PIP& pip);

    /**
     * @brief Evaluates an access request.
     *
     * @param[in] userId       Subject user ID.
     * @param[in] resourceId   Resource being accessed.
     * @param[in] action       Requested action (e.g. "read", "write", "*").
     * @param[in] resourceType Resource type (e.g. "file_share").
     * @return Decision with allow/deny and reason.
     */
    Decision evaluate(qint64 userId, qint64 resourceId, const QString& action, const QString& resourceType);

    /// Policy management.
    QVector<pip::PolicyRecord> listPolicies(int page = 1, int pageSize = 50);
    int countPolicies();
    qint64 createPolicy(const pip::PolicyRecord& policy, qint64 createdBy);
    bool updatePolicy(qint64 id, const pip::PolicyRecord& policy);
    bool deletePolicy(qint64 id);

private:
    pip::PIP& m_pip;
};

} // namespace pdp
