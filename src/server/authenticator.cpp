#include <server/authenticator.h>

#include <QCryptographicHash>
#include <QDateTime>
#include <QRandomGenerator>
#include <QSqlError>
#include <QSqlQuery>

namespace auth
{

Authenticator::Authenticator(db::DbLayer& db, const server::ServerConfig& cfg) : m_db(db), m_cfg(cfg)
{
}

// ---------------------------------------------------------------------------
// Password hashing: PBKDF2-HMAC-SHA256
// ---------------------------------------------------------------------------

QString Authenticator::hashPassword(const QString& password, const QString& salt)
{
    // Generate salt if not provided.
    QByteArray actualSalt;
    if (salt.isEmpty())
    {
        actualSalt.resize(32);
        auto* rng = QRandomGenerator::global();
        auto* p = reinterpret_cast<quint32*>(actualSalt.data());
        for (int i = 0; i < 8; ++i)
        {
            p[i] = rng->generate();
        }
    }
    else
    {
        actualSalt = QByteArray::fromHex(salt.toUtf8());
    }

    // PBKDF2-HMAC-SHA256 with 100 000 iterations.
    constexpr int iterations = 100000;
    constexpr int keyLength = 32; // 256 bits

    QByteArray passwordBytes = password.toUtf8();
    QByteArray result;
    result.resize(keyLength);

    // U1 = HMAC(password, salt || INT(1))
    QByteArray block = actualSalt + QByteArray("\x00\x00\x00\x01", 4);
    QByteArray u = QCryptographicHash::hash(passwordBytes + block, QCryptographicHash::Sha256);
    result = u;

    // U2..Uc
    for (int i = 1; i < iterations; ++i)
    {
        u = QCryptographicHash::hash(passwordBytes + u, QCryptographicHash::Sha256);
        for (int j = 0; j < keyLength; ++j)
        {
            result[j] ^= u[j];
        }
    }

    return QString::fromLatin1(result.toHex()) + QStringLiteral(":") + QString::fromLatin1(actualSalt.toHex());
}

// ---------------------------------------------------------------------------
// Token generation and hashing
// ---------------------------------------------------------------------------

QString Authenticator::generateToken()
{
    QByteArray token;
    token.resize(32);
    auto* rng = QRandomGenerator::global();
    auto* p = reinterpret_cast<quint32*>(token.data());
    for (int i = 0; i < 8; ++i)
    {
        p[i] = rng->generate();
    }
    return QString::fromLatin1(token.toHex());
}

QString Authenticator::hashToken(const QString& token)
{
    return QString::fromLatin1(QCryptographicHash::hash(token.toUtf8(), QCryptographicHash::Sha256).toHex());
}

// ---------------------------------------------------------------------------
// Authentication
// ---------------------------------------------------------------------------

AuthResult Authenticator::authenticate(const QString& username, const QString& password)
{
    AuthResult result;

    auto user = m_db.findUserByUsername(username);
    if (!user)
    {
        result.error_code = QStringLiteral("AUTH_DENIED");
        return result;
    }

    // Check if account is active.
    if (!user->is_active)
    {
        result.error_code = QStringLiteral("ACCOUNT_INACTIVE");
        return result;
    }

    // Check lockout.
    if (!user->locked_until.isEmpty())
    {
        QDateTime lockedUntil = QDateTime::fromString(user->locked_until, Qt::ISODate);
        if (lockedUntil.isValid() && lockedUntil > QDateTime::currentDateTimeUtc())
        {
            result.error_code = QStringLiteral("ACCOUNT_LOCKED");
            return result;
        }
    }

    // Verify password.
    const QString storedHash = user->password_hash + QStringLiteral(":") + user->salt;
    const QString computedHash = hashPassword(password, user->salt);

    if (storedHash != computedHash)
    {
        // Increment failed attempts.
        m_db.incrementFailedAttempts(user->id);

        auto updatedUser = m_db.findUserById(user->id);
        if (updatedUser && updatedUser->failed_attempts >= m_cfg.max_failed_attempts)
        {
            m_db.lockUser(user->id, m_cfg.lockout_minutes);
        }

        result.error_code = QStringLiteral("AUTH_DENIED");
        return result;
    }

    // Success: reset failed attempts, update last login.
    m_db.resetFailedAttempts(user->id);
    m_db.updateLastLogin(user->id);

    // Get subject attrs.
    auto attrs = m_db.getSubjectAttrs(user->id);

    // Generate token and create session.
    const QString token = generateToken();
    const QString tokenHash = hashToken(token);
    const qint64 sessionId = m_db.createSession(user->id, tokenHash, m_cfg.session_lifetime_h);

    if (sessionId < 0)
    {
        result.error_code = QStringLiteral("INTERNAL_ERROR");
        return result;
    }

    result.success = true;
    result.token = token;
    result.user_id = user->id;
    result.username = user->username;
    result.full_name = user->full_name;
    result.role = attrs ? attrs->role : QStringLiteral("user");
    result.clearance_level = attrs ? attrs->clearance_level : 0;
    result.department = attrs ? attrs->department : QString();

    return result;
}

// ---------------------------------------------------------------------------
// Token verification
// ---------------------------------------------------------------------------

std::optional<db::SubjectAttrs> Authenticator::verifyToken(const QString& token)
{
    const QString tokenHash = hashToken(token);
    auto session = m_db.findSessionByTokenHash(tokenHash);
    if (!session)
    {
        return std::nullopt;
    }

    auto user = m_db.findUserById(session->user_id);
    if (!user || !user->is_active)
    {
        return std::nullopt;
    }

    return m_db.getSubjectAttrs(user->id);
}

// ---------------------------------------------------------------------------
// Logout
// ---------------------------------------------------------------------------

bool Authenticator::logout(const QString& token)
{
    const QString tokenHash = hashToken(token);
    auto session = m_db.findSessionByTokenHash(tokenHash);
    if (!session)
    {
        return false;
    }
    return m_db.revokeSession(session->id);
}

} // namespace auth
