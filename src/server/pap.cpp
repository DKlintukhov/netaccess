#include <server/pap.h>

namespace pap
{

PAP::PAP(pdp::PDP& pdp, pip::PIP& pip) : m_pdp(pdp), m_pip(pip)
{
}

QVector<pip::PolicyRecord> PAP::listPolicies(int page, int pageSize)
{
    return m_pdp.listPolicies(page, pageSize);
}

int PAP::countPolicies()
{
    return m_pdp.countPolicies();
}

qint64 PAP::createPolicy(const pip::PolicyRecord& policy, qint64 createdBy)
{
    return m_pdp.createPolicy(policy, createdBy);
}

bool PAP::updatePolicy(qint64 id, const pip::PolicyRecord& policy)
{
    return m_pdp.updatePolicy(id, policy);
}

bool PAP::deletePolicy(qint64 id)
{
    return m_pdp.deletePolicy(id);
}

} // namespace pap
