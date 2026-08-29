/**
 * @file pip.h
 * @brief Policy Information Point (PIP) — provides attribute data from PostgreSQL.
 *
 * Wraps QPSQL with prepared statements for all CRUD operations.
 * All user-supplied data is bound via placeholders — no string
 * concatenation in SQL (protection against SQL injection).
 */

#pragma once

#include <QSqlDatabase>
#include <QVector>
#include <optional>

#include <server/config.h>

namespace pip
{

// ---------------------------------------------------------------------------
// DTOs returned by the data layer (maps to DB rows).
// ---------------------------------------------------------------------------

struct UserRecord
{
    qint64 id = 0;
    QString username;
    QString password_hash;
    QString salt;
    QString full_name;
    QString position;
    bool is_active = true;
    int failed_attempts = 0;
    QString locked_until;
    QString last_login_at;
};

struct SessionRecord
{
    qint64 id = 0;
    qint64 user_id = 0;
    QString token_hash;
    QString created_at;
    QString expires_at;
    QString revoked_at;
};

struct SubjectAttrs
{
    qint64 user_id = 0;
    QString role = QStringLiteral("user");
    int clearance_level = 0;
    QString department;
};

struct ResourceRecord
{
    qint64 id = 0;
    QString name;
    QString description;
    QString resource_type;
    QString address;
    qint64 owner_id = 0;
    bool is_active = true;
};

struct PolicyRecord
{
    qint64 id = 0;
    QString name;
    bool enabled = true;
    QString action;
    QString role_required;
    QString department_required;
    int min_clearance = -1;
    QString resource_type;
    qint64 subject_id = 0;
    qint64 resource_id = 0;
    int priority = 0;
    qint64 created_by = 0;
};

struct AuditRecord
{
    qint64 id = 0;
    QString ts;
    qint64 actor_id = 0;
    QString actor_name;
    QString action;
    QString target_type;
    qint64 target_id = 0;
    QString result;
    QString details;
};

struct AuditFilter
{
    QString from;
    QString to;
    qint64 actor_id = 0;
    QString action;
    int page = 1;
    int page_size = 100;
};

// ---------------------------------------------------------------------------
// Policy Information Point — attribute provider backed by PostgreSQL.
// ---------------------------------------------------------------------------

/**
 * @brief Policy Information Point (PIP).
 *
 * Provides subject, resource, and policy attributes to the PDP.
 * All queries use prepared statements.
 */
class PIP
{
public:
    PIP() = default;
    ~PIP();

    PIP(const PIP&) = delete;
    PIP& operator=(const PIP&) = delete;

    bool open(const server::ServerConfig& cfg);
    void close();
    bool isConnected() const;
    bool applyMigrations(const QString& schemaPath, const QString& seedPath);

    // ----- Users -----------------------------------------------------------

    std::optional<UserRecord> findUserByUsername(const QString& username);
    std::optional<UserRecord> findUserById(qint64 id);

    qint64 createUser(const QString& username, const QString& passwordHash, const QString& salt,
                      const QString& fullName, const QString& position = {},
                      const QString& role = QStringLiteral("user"), int clearanceLevel = 0,
                      const QString& department = {});

    bool updateUser(qint64 id, const QString& fullName, const QString& position, bool isActive);
    bool deleteUser(qint64 id);
    QVector<UserRecord> listUsers(int page = 1, int pageSize = 50);
    int countUsers();

    bool incrementFailedAttempts(qint64 userId);
    bool resetFailedAttempts(qint64 userId);
    bool lockUser(qint64 userId, int minutes);
    bool updateLastLogin(qint64 userId);

    // ----- Subject attrs ---------------------------------------------------

    std::optional<SubjectAttrs> getSubjectAttrs(qint64 userId);
    bool updateSubjectAttrs(qint64 userId, const QString& role, int clearanceLevel, const QString& department);

    // ----- Sessions --------------------------------------------------------

    qint64 createSession(qint64 userId, const QString& tokenHash, int lifetimeHours);
    std::optional<SessionRecord> findSessionByTokenHash(const QString& tokenHash);
    bool revokeSession(qint64 sessionId);
    bool revokeAllSessions(qint64 userId);
    int countActiveSessions();

    // ----- Resources -------------------------------------------------------

    qint64 createResource(const QString& name, const QString& description, const QString& resourceType,
                          const QString& address, qint64 ownerId);
    std::optional<ResourceRecord> getResource(qint64 id);
    bool updateResource(qint64 id, const QString& name, const QString& description, const QString& resourceType,
                        const QString& address, bool isActive);
    bool deleteResource(qint64 id);
    QVector<ResourceRecord> listResources(const QString& type = {}, int page = 1, int pageSize = 50);
    int countResources(const QString& type = {});

    // ----- Policies --------------------------------------------------------

    qint64 createPolicy(const QString& name, const QString& action, bool enabled, int priority,
                        const QString& roleRequired = {}, const QString& departmentRequired = {}, int minClearance = -1,
                        const QString& resourceType = {}, qint64 subjectId = 0, qint64 resourceId = 0,
                        qint64 createdBy = 0);
    std::optional<PolicyRecord> getPolicy(qint64 id);
    bool updatePolicy(qint64 id, const QString& name, const QString& action, bool enabled, int priority,
                      const QString& roleRequired = {}, const QString& departmentRequired = {}, int minClearance = -1,
                      const QString& resourceType = {}, qint64 subjectId = 0, qint64 resourceId = 0);
    bool deletePolicy(qint64 id);
    QVector<PolicyRecord> listPolicies(int page = 1, int pageSize = 50);
    int countPolicies();
    QVector<PolicyRecord> findEnabledPolicies(const QString& action);

    // ----- Audit -----------------------------------------------------------

    void writeAuditLog(qint64 actorId, const QString& actorName, const QString& action, const QString& targetType = {},
                       qint64 targetId = 0, const QString& result = QStringLiteral("ok"), const QString& details = {});
    QVector<AuditRecord> queryAuditLog(const AuditFilter& filter);
    int countAuditLog(const AuditFilter& filter);

private:
    bool runSqlFile(const QString& path);
    QSqlDatabase m_db;
};

} // namespace pip
