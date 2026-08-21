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
#include <server/db_layer.h>

namespace auth
{

/**
 * @brief Result of an authentication attempt.
 */
struct AuthResult
{
    bool success = false;
    QString token; ///< Raw session token (only on success).
    qint64 user_id = 0;
    QString username;
    QString full_name;
    QString role;
    int clearance_level = 0;
    QString department;
    QString error_code; ///< protocol::ResultCode string on failure.
};

/**
 * @brief Authenticator: password verification, token creation, lockout.
 */
class Authenticator
{
public:
    explicit Authenticator(db::DbLayer& db, const server::ServerConfig& cfg);

    /// Verifies credentials and creates a session. Returns AuthResult.
    AuthResult authenticate(const QString& username, const QString& password);

    /// Verifies a session token and returns user info.
    std::optional<db::SubjectAttrs> verifyToken(const QString& token);

    /// Ends a session (LOGOUT).
    bool logout(const QString& token);

    // ----- Password utilities ----------------------------------------------

    /// Hashes a password with PBKDF2-HMAC-SHA256. Returns "hash_hex:salt_hex".
    static QString hashPassword(const QString& password, const QString& salt = {});

    /// Generates a random 256-bit token and returns it as hex.
    static QString generateToken();

    /// Returns the SHA-256 hex hash of a token (for DB storage).
    static QString hashToken(const QString& token);

private:
    db::DbLayer& m_db;
    server::ServerConfig m_cfg;
};

} // namespace auth
