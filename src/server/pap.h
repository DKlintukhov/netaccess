/**
 * @file pap.h
 * @brief Policy Administration Point (PAP) — manages ABAC policies.
 *
 * Provides CRUD operations for policy management.  Policies are stored
 * in PostgreSQL via the PIP and evaluated by the PDP.
 */

#pragma once

#include <QVector>

#include <server/pdp.h>
#include <server/pip.h>

namespace pap
{

/**
 * @brief Policy Administration Point — manages policy lifecycle.
 */
class PAP
{
public:
    explicit PAP(pdp::PDP& pdp, pip::PIP& pip);

    /// Policy CRUD (delegates to PDP which uses PIP for storage).
    QVector<pip::PolicyRecord> listPolicies(int page = 1, int pageSize = 50);
    int countPolicies();
    qint64 createPolicy(const pip::PolicyRecord& policy, qint64 createdBy);
    bool updatePolicy(qint64 id, const pip::PolicyRecord& policy);
    bool deletePolicy(qint64 id);

private:
    pdp::PDP& m_pdp;
    pip::PIP& m_pip;
};

} // namespace pap
