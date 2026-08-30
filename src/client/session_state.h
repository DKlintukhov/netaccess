/**
 * @file session_state.h
 * @brief Client-side session state.
 *
 * Stores the authentication token and current user info.
 * Exposed to QML for reactive UI updates.
 */

#pragma once

#include <QObject>
#include <QString>

namespace client
{

/**
 * @brief Session state exposed to QML.
 */
class SessionState : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool authenticated READ isAuthenticated NOTIFY authenticatedChanged)
    Q_PROPERTY(QString token READ token NOTIFY tokenChanged)
    Q_PROPERTY(qint64 userId READ userId NOTIFY userInfoChanged)
    Q_PROPERTY(QString username READ username NOTIFY userInfoChanged)
    Q_PROPERTY(QString fullName READ fullName NOTIFY userInfoChanged)
    Q_PROPERTY(QString role READ role NOTIFY userInfoChanged)
    Q_PROPERTY(int clearanceLevel READ clearanceLevel NOTIFY userInfoChanged)
    Q_PROPERTY(QString department READ department NOTIFY userInfoChanged)

public:
    explicit SessionState(QObject* parent = nullptr);

    bool isAuthenticated() const;
    QString token() const;
    qint64 userId() const;
    QString username() const;
    QString fullName() const;
    QString role() const;
    int clearanceLevel() const;
    QString department() const;

    /// Sets the session after successful AUTH.
    Q_INVOKABLE void setAuth(const QString& token, qint64 userId, const QString& username, const QString& fullName,
                             const QString& role, int clearanceLevel, const QString& department);

    /// Clears the session (logout).
    Q_INVOKABLE void clear();

    /// Returns true if the user has admin role.
    Q_INVOKABLE bool isAdmin() const;

signals:
    void authenticatedChanged();
    void tokenChanged();
    void userInfoChanged();

private:
    QString m_token;
    QString m_username;
    QString m_fullName;
    QString m_role;
    QString m_department;
    qint64 m_userId = 0;
    int m_clearanceLevel = 0;
};

} // namespace client
