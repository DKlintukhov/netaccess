/**
 * @file authenticator.h
 * @brief Authentication logic for netaccess server.
 *
 * Password hashing: PBKDF2-HMAC-SHA256 (100 000 iterations).
 * Token generation: 256-bit random token, SHA-256 hash stored in DB.
 * Brute-force protection: 5 failed attempts -> lockout 15 min.
 */

#pragma once

#include <QString>
#include <optional>

#include <server/config.h>
#include <server/pip.h>

namespace auth
{

/**
 * @brief Result of an authentication attempt.
 */
struct AuthResult
{
    bool success = false;
    QString token;
    qint64 user_id = 0;
    QString username;
    QString full_name;
    QString role;
    int clearance_level = 0;
    QString department;
    QString error_code;
};

/**
 * @brief Authenticator: password verification, token creation, lockout.
 */
class Authenticator
{
public:
    explicit Authenticator(pip::PIP& pip, const server::ServerConfig& cfg);

    AuthResult authenticate(const QString& username, const QString& password);
    std::optional<pip::SubjectAttrs> verifyToken(const QString& token);
    bool logout(const QString& token);

    static QString hashPassword(const QString& password, const QString& salt = {});
    static QString generateToken();
    static QString hashToken(const QString& token);

private:
    pip::PIP& m_pip;
    server::ServerConfig m_cfg;
};

} // namespace auth
