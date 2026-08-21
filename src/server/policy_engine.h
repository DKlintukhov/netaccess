/**
 * @file policy_engine.h
 * @brief ABAC policy evaluation engine backed by PostgreSQL.
 *
 * Loads policies from the database and evaluates access decisions.
 * Pure ABAC: deny by default.
 */

#pragma once

#include <QString>
#include <QVector>

#include <server/db_layer.h>

namespace policy
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
 * @brief ABAC policy engine.
 */
class PolicyEngine
{
public:
    explicit PolicyEngine(db::DbLayer& db);

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
    QVector<db::PolicyRecord> listPolicies(int page = 1, int pageSize = 50);
    int countPolicies();
    qint64 createPolicy(const db::PolicyRecord& policy, qint64 createdBy);
    bool updatePolicy(qint64 id, const db::PolicyRecord& policy);
    bool deletePolicy(qint64 id);

private:
    db::DbLayer& m_db;
};

} // namespace policy
