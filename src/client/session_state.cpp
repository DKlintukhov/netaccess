#include <client/session_state.h>

namespace client
{

SessionState::SessionState(QObject* parent) : QObject(parent)
{
}

bool SessionState::isAuthenticated() const
{
    return !m_token.isEmpty();
}

QString SessionState::token() const
{
    return m_token;
}

qint64 SessionState::userId() const
{
    return m_userId;
}

QString SessionState::username() const
{
    return m_username;
}

QString SessionState::fullName() const
{
    return m_fullName;
}

QString SessionState::role() const
{
    return m_role;
}

int SessionState::clearanceLevel() const
{
    return m_clearanceLevel;
}

QString SessionState::department() const
{
    return m_department;
}

void SessionState::setAuth(const QString& token, qint64 userId, const QString& username, const QString& fullName,
                           const QString& role, int clearanceLevel, const QString& department)
{
    m_token = token;
    m_userId = userId;
    m_username = username;
    m_fullName = fullName;
    m_role = role;
    m_clearanceLevel = clearanceLevel;
    m_department = department;

    emit tokenChanged();
    emit userInfoChanged();
    emit authenticatedChanged();
}

void SessionState::clear()
{
    m_token.clear();
    m_userId = 0;
    m_username.clear();
    m_fullName.clear();
    m_role.clear();
    m_clearanceLevel = 0;
    m_department.clear();

    emit tokenChanged();
    emit userInfoChanged();
    emit authenticatedChanged();
}

bool SessionState::isAdmin() const
{
    return m_role == QStringLiteral("admin");
}

} // namespace client
