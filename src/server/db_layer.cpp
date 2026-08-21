#include <server/db_layer.h>

#include <QDateTime>
#include <QFile>
#include <QSqlError>
#include <QSqlQuery>
#include <QTextStream>

namespace db
{

DbLayer::~DbLayer()
{
    close();
}

bool DbLayer::open(const server::ServerConfig& cfg)
{
    m_db = QSqlDatabase::addDatabase(QStringLiteral("QPSQL"));
    m_db.setHostName(cfg.db_host);
    m_db.setPort(cfg.db_port);
    m_db.setDatabaseName(cfg.db_name);
    m_db.setUserName(cfg.db_user);
    m_db.setPassword(cfg.db_password);

    if (!m_db.open())
    {
        return false;
    }

    QSqlQuery q(m_db);
    q.exec(QStringLiteral("SET client_min_messages = warning;"));
    return true;
}

void DbLayer::close()
{
    if (m_db.isOpen())
    {
        m_db.close();
    }
}

bool DbLayer::isConnected() const
{
    return m_db.isOpen();
}

bool DbLayer::runSqlFile(const QString& path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        return false;
    }

    const QString sql = QString::fromUtf8(file.readAll());
    QSqlQuery q(m_db);
    return q.exec(sql);
}

bool DbLayer::applyMigrations(const QString& schemaPath, const QString& seedPath)
{
    if (!runSqlFile(schemaPath))
    {
        return false;
    }
    return runSqlFile(seedPath);
}

// ---------------------------------------------------------------------------
// Users
// ---------------------------------------------------------------------------

std::optional<UserRecord> DbLayer::findUserByUsername(const QString& username)
{
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral("SELECT id, username, password_hash, salt, full_name, position, is_active, "
                             "failed_attempts, locked_until, last_login_at "
                             "FROM users WHERE username = :username"));
    q.bindValue(QStringLiteral(":username"), username);

    if (!q.exec() || !q.next())
    {
        return std::nullopt;
    }

    UserRecord r;
    r.id = q.value(0).toLongLong();
    r.username = q.value(1).toString();
    r.password_hash = q.value(2).toString();
    r.salt = q.value(3).toString();
    r.full_name = q.value(4).toString();
    r.position = q.value(5).toString();
    r.is_active = q.value(6).toBool();
    r.failed_attempts = q.value(7).toInt();
    r.locked_until = q.value(8).toString();
    r.last_login_at = q.value(9).toString();
    return r;
}

std::optional<UserRecord> DbLayer::findUserById(qint64 id)
{
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral("SELECT id, username, password_hash, salt, full_name, position, is_active, "
                             "failed_attempts, locked_until, last_login_at "
                             "FROM users WHERE id = :id"));
    q.bindValue(QStringLiteral(":id"), id);

    if (!q.exec() || !q.next())
    {
        return std::nullopt;
    }

    UserRecord r;
    r.id = q.value(0).toLongLong();
    r.username = q.value(1).toString();
    r.password_hash = q.value(2).toString();
    r.salt = q.value(3).toString();
    r.full_name = q.value(4).toString();
    r.position = q.value(5).toString();
    r.is_active = q.value(6).toBool();
    r.failed_attempts = q.value(7).toInt();
    r.locked_until = q.value(8).toString();
    r.last_login_at = q.value(9).toString();
    return r;
}

qint64 DbLayer::createUser(const QString& username, const QString& passwordHash, const QString& salt,
                           const QString& fullName, const QString& position, const QString& role, int clearanceLevel,
                           const QString& department)
{
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral("INSERT INTO users (username, password_hash, salt, full_name, position) "
                             "VALUES (:username, :password_hash, :salt, :full_name, :position) "
                             "RETURNING id"));
    q.bindValue(QStringLiteral(":username"), username);
    q.bindValue(QStringLiteral(":password_hash"), passwordHash);
    q.bindValue(QStringLiteral(":salt"), salt);
    q.bindValue(QStringLiteral(":full_name"), fullName);
    q.bindValue(QStringLiteral(":position"), position);

    if (!q.exec() || !q.next())
    {
        return -1;
    }

    const qint64 userId = q.value(0).toLongLong();

    QSqlQuery sa(m_db);
    sa.prepare(QStringLiteral("INSERT INTO subject_attrs (user_id, role, clearance_level, department) "
                              "VALUES (:user_id, :role, :clearance_level, :department)"));
    sa.bindValue(QStringLiteral(":user_id"), userId);
    sa.bindValue(QStringLiteral(":role"), role);
    sa.bindValue(QStringLiteral(":clearance_level"), clearanceLevel);
    sa.bindValue(QStringLiteral(":department"), department);

    if (!sa.exec())
    {
        return -1;
    }

    return userId;
}

bool DbLayer::updateUser(qint64 id, const QString& fullName, const QString& position, bool isActive)
{
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral("UPDATE users SET full_name = :full_name, position = :position, "
                             "is_active = :is_active, updated_at = now() WHERE id = :id"));
    q.bindValue(QStringLiteral(":full_name"), fullName);
    q.bindValue(QStringLiteral(":position"), position);
    q.bindValue(QStringLiteral(":is_active"), isActive);
    q.bindValue(QStringLiteral(":id"), id);
    return q.exec();
}

bool DbLayer::deleteUser(qint64 id)
{
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral("DELETE FROM users WHERE id = :id"));
    q.bindValue(QStringLiteral(":id"), id);
    return q.exec();
}

QVector<UserRecord> DbLayer::listUsers(int page, int pageSize)
{
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral("SELECT id, username, password_hash, salt, full_name, position, is_active, "
                             "failed_attempts, locked_until, last_login_at "
                             "FROM users ORDER BY id LIMIT :limit OFFSET :offset"));
    q.bindValue(QStringLiteral(":limit"), pageSize);
    q.bindValue(QStringLiteral(":offset"), (page - 1) * pageSize);
    q.exec();

    QVector<UserRecord> users;
    while (q.next())
    {
        UserRecord r;
        r.id = q.value(0).toLongLong();
        r.username = q.value(1).toString();
        r.password_hash = q.value(2).toString();
        r.salt = q.value(3).toString();
        r.full_name = q.value(4).toString();
        r.position = q.value(5).toString();
        r.is_active = q.value(6).toBool();
        r.failed_attempts = q.value(7).toInt();
        r.locked_until = q.value(8).toString();
        r.last_login_at = q.value(9).toString();
        users.append(r);
    }
    return users;
}

int DbLayer::countUsers()
{
    QSqlQuery q(m_db);
    q.exec(QStringLiteral("SELECT count(*) FROM users"));
    return q.next() ? q.value(0).toInt() : 0;
}

bool DbLayer::incrementFailedAttempts(qint64 userId)
{
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral("UPDATE users SET failed_attempts = failed_attempts + 1 WHERE id = :id"));
    q.bindValue(QStringLiteral(":id"), userId);
    return q.exec();
}

bool DbLayer::resetFailedAttempts(qint64 userId)
{
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral("UPDATE users SET failed_attempts = 0, locked_until = NULL WHERE id = :id"));
    q.bindValue(QStringLiteral(":id"), userId);
    return q.exec();
}

bool DbLayer::lockUser(qint64 userId, int minutes)
{
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral("UPDATE users SET locked_until = now() + (:minutes || ' minutes')::interval "
                             "WHERE id = :id"));
    q.bindValue(QStringLiteral(":minutes"), minutes);
    q.bindValue(QStringLiteral(":id"), userId);
    return q.exec();
}

bool DbLayer::updateLastLogin(qint64 userId)
{
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral("UPDATE users SET last_login_at = now() WHERE id = :id"));
    q.bindValue(QStringLiteral(":id"), userId);
    return q.exec();
}

// ---------------------------------------------------------------------------
// Subject attrs
// ---------------------------------------------------------------------------

std::optional<SubjectAttrs> DbLayer::getSubjectAttrs(qint64 userId)
{
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral("SELECT user_id, role, clearance_level, department FROM subject_attrs "
                             "WHERE user_id = :user_id"));
    q.bindValue(QStringLiteral(":user_id"), userId);

    if (!q.exec() || !q.next())
    {
        return std::nullopt;
    }

    SubjectAttrs a;
    a.user_id = q.value(0).toLongLong();
    a.role = q.value(1).toString();
    a.clearance_level = q.value(2).toInt();
    a.department = q.value(3).toString();
    return a;
}

bool DbLayer::updateSubjectAttrs(qint64 userId, const QString& role, int clearanceLevel, const QString& department)
{
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral("UPDATE subject_attrs SET role = :role, clearance_level = :clearance_level, "
                             "department = :department WHERE user_id = :user_id"));
    q.bindValue(QStringLiteral(":role"), role);
    q.bindValue(QStringLiteral(":clearance_level"), clearanceLevel);
    q.bindValue(QStringLiteral(":department"), department);
    q.bindValue(QStringLiteral(":user_id"), userId);
    return q.exec();
}

// ---------------------------------------------------------------------------
// Sessions
// ---------------------------------------------------------------------------

qint64 DbLayer::createSession(qint64 userId, const QString& tokenHash, int lifetimeHours)
{
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral("INSERT INTO sessions (user_id, token_hash, expires_at) "
                             "VALUES (:user_id, :token_hash, now() + (:hours || ' hours')::interval) "
                             "RETURNING id"));
    q.bindValue(QStringLiteral(":user_id"), userId);
    q.bindValue(QStringLiteral(":token_hash"), tokenHash);
    q.bindValue(QStringLiteral(":hours"), lifetimeHours);

    if (!q.exec() || !q.next())
    {
        return -1;
    }
    return q.value(0).toLongLong();
}

std::optional<SessionRecord> DbLayer::findSessionByTokenHash(const QString& tokenHash)
{
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral("SELECT id, user_id, token_hash, created_at, expires_at, revoked_at "
                             "FROM sessions WHERE token_hash = :token_hash AND expires_at > now() "
                             "AND revoked_at IS NULL"));
    q.bindValue(QStringLiteral(":token_hash"), tokenHash);

    if (!q.exec() || !q.next())
    {
        return std::nullopt;
    }

    SessionRecord s;
    s.id = q.value(0).toLongLong();
    s.user_id = q.value(1).toLongLong();
    s.token_hash = q.value(2).toString();
    s.created_at = q.value(3).toString();
    s.expires_at = q.value(4).toString();
    s.revoked_at = q.value(5).toString();
    return s;
}

bool DbLayer::revokeSession(qint64 sessionId)
{
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral("UPDATE sessions SET revoked_at = now() WHERE id = :id AND revoked_at IS NULL"));
    q.bindValue(QStringLiteral(":id"), sessionId);
    return q.exec();
}

bool DbLayer::revokeAllSessions(qint64 userId)
{
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral("UPDATE sessions SET revoked_at = now() WHERE user_id = :user_id AND revoked_at IS NULL"));
    q.bindValue(QStringLiteral(":user_id"), userId);
    return q.exec();
}

int DbLayer::countActiveSessions()
{
    QSqlQuery q(m_db);
    q.exec(QStringLiteral("SELECT count(*) FROM sessions WHERE expires_at > now() AND revoked_at IS NULL"));
    return q.next() ? q.value(0).toInt() : 0;
}

// ---------------------------------------------------------------------------
// Resources
// ---------------------------------------------------------------------------

qint64 DbLayer::createResource(const QString& name, const QString& description, const QString& resourceType,
                               const QString& address, qint64 ownerId)
{
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral("INSERT INTO resources (name, description, resource_type, address, owner_id) "
                             "VALUES (:name, :description, :resource_type, :address, :owner_id) "
                             "RETURNING id"));
    q.bindValue(QStringLiteral(":name"), name);
    q.bindValue(QStringLiteral(":description"), description);
    q.bindValue(QStringLiteral(":resource_type"), resourceType);
    q.bindValue(QStringLiteral(":address"), address);
    q.bindValue(QStringLiteral(":owner_id"), ownerId > 0 ? ownerId : QVariant());

    if (!q.exec() || !q.next())
    {
        return -1;
    }
    return q.value(0).toLongLong();
}

std::optional<ResourceRecord> DbLayer::getResource(qint64 id)
{
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral("SELECT id, name, description, resource_type, address, owner_id, is_active "
                             "FROM resources WHERE id = :id"));
    q.bindValue(QStringLiteral(":id"), id);

    if (!q.exec() || !q.next())
    {
        return std::nullopt;
    }

    ResourceRecord r;
    r.id = q.value(0).toLongLong();
    r.name = q.value(1).toString();
    r.description = q.value(2).toString();
    r.resource_type = q.value(3).toString();
    r.address = q.value(4).toString();
    r.owner_id = q.value(5).toLongLong();
    r.is_active = q.value(6).toBool();
    return r;
}

bool DbLayer::updateResource(qint64 id, const QString& name, const QString& description, const QString& resourceType,
                             const QString& address, bool isActive)
{
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral("UPDATE resources SET name = :name, description = :description, "
                             "resource_type = :resource_type, address = :address, is_active = :is_active, "
                             "updated_at = now() WHERE id = :id"));
    q.bindValue(QStringLiteral(":name"), name);
    q.bindValue(QStringLiteral(":description"), description);
    q.bindValue(QStringLiteral(":resource_type"), resourceType);
    q.bindValue(QStringLiteral(":address"), address);
    q.bindValue(QStringLiteral(":is_active"), isActive);
    q.bindValue(QStringLiteral(":id"), id);
    return q.exec();
}

bool DbLayer::deleteResource(qint64 id)
{
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral("DELETE FROM resources WHERE id = :id"));
    q.bindValue(QStringLiteral(":id"), id);
    return q.exec();
}

QVector<ResourceRecord> DbLayer::listResources(const QString& type, int page, int pageSize)
{
    QSqlQuery q(m_db);
    if (type.isEmpty())
    {
        q.prepare(QStringLiteral("SELECT id, name, description, resource_type, address, owner_id, is_active "
                                 "FROM resources ORDER BY id LIMIT :limit OFFSET :offset"));
    }
    else
    {
        q.prepare(QStringLiteral("SELECT id, name, description, resource_type, address, owner_id, is_active "
                                 "FROM resources WHERE resource_type = :type ORDER BY id LIMIT :limit OFFSET :offset"));
        q.bindValue(QStringLiteral(":type"), type);
    }
    q.bindValue(QStringLiteral(":limit"), pageSize);
    q.bindValue(QStringLiteral(":offset"), (page - 1) * pageSize);
    q.exec();

    QVector<ResourceRecord> resources;
    while (q.next())
    {
        ResourceRecord r;
        r.id = q.value(0).toLongLong();
        r.name = q.value(1).toString();
        r.description = q.value(2).toString();
        r.resource_type = q.value(3).toString();
        r.address = q.value(4).toString();
        r.owner_id = q.value(5).toLongLong();
        r.is_active = q.value(6).toBool();
        resources.append(r);
    }
    return resources;
}

int DbLayer::countResources(const QString& type)
{
    QSqlQuery q(m_db);
    if (type.isEmpty())
    {
        q.exec(QStringLiteral("SELECT count(*) FROM resources"));
    }
    else
    {
        q.prepare(QStringLiteral("SELECT count(*) FROM resources WHERE resource_type = :type"));
        q.bindValue(QStringLiteral(":type"), type);
        q.exec();
    }
    return q.next() ? q.value(0).toInt() : 0;
}

// ---------------------------------------------------------------------------
// Policies
// ---------------------------------------------------------------------------

qint64 DbLayer::createPolicy(const QString& name, const QString& action, bool enabled, int priority,
                             const QString& roleRequired, const QString& departmentRequired, int minClearance,
                             const QString& resourceType, qint64 subjectId, qint64 resourceId, qint64 createdBy)
{
    QSqlQuery q(m_db);
    q.prepare(
        QStringLiteral("INSERT INTO policies (name, action, enabled, priority, role_required, "
                       "department_required, min_clearance, resource_type, subject_id, resource_id, created_by) "
                       "VALUES (:name, :action, :enabled, :priority, :role_required, "
                       ":department_required, :min_clearance, :resource_type, :subject_id, :resource_id, :created_by) "
                       "RETURNING id"));
    q.bindValue(QStringLiteral(":name"), name);
    q.bindValue(QStringLiteral(":action"), action);
    q.bindValue(QStringLiteral(":enabled"), enabled);
    q.bindValue(QStringLiteral(":priority"), priority);
    q.bindValue(QStringLiteral(":role_required"), roleRequired.isEmpty() ? QVariant() : roleRequired);
    q.bindValue(QStringLiteral(":department_required"), departmentRequired.isEmpty() ? QVariant() : departmentRequired);
    q.bindValue(QStringLiteral(":min_clearance"), minClearance < 0 ? QVariant() : minClearance);
    q.bindValue(QStringLiteral(":resource_type"), resourceType.isEmpty() ? QVariant() : resourceType);
    q.bindValue(QStringLiteral(":subject_id"), subjectId > 0 ? subjectId : QVariant());
    q.bindValue(QStringLiteral(":resource_id"), resourceId > 0 ? resourceId : QVariant());
    q.bindValue(QStringLiteral(":created_by"), createdBy > 0 ? createdBy : QVariant());

    if (!q.exec() || !q.next())
    {
        return -1;
    }
    return q.value(0).toLongLong();
}

std::optional<PolicyRecord> DbLayer::getPolicy(qint64 id)
{
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral("SELECT id, name, enabled, action, role_required, department_required, "
                             "min_clearance, resource_type, subject_id, resource_id, priority, created_by "
                             "FROM policies WHERE id = :id"));
    q.bindValue(QStringLiteral(":id"), id);

    if (!q.exec() || !q.next())
    {
        return std::nullopt;
    }

    PolicyRecord p;
    p.id = q.value(0).toLongLong();
    p.name = q.value(1).toString();
    p.enabled = q.value(2).toBool();
    p.action = q.value(3).toString();
    p.role_required = q.value(4).toString();
    p.department_required = q.value(5).toString();
    p.min_clearance = q.value(6).isNull() ? -1 : q.value(6).toInt();
    p.resource_type = q.value(7).toString();
    p.subject_id = q.value(8).isNull() ? 0 : q.value(8).toLongLong();
    p.resource_id = q.value(9).isNull() ? 0 : q.value(9).toLongLong();
    p.priority = q.value(10).toInt();
    p.created_by = q.value(11).isNull() ? 0 : q.value(11).toLongLong();
    return p;
}

bool DbLayer::updatePolicy(qint64 id, const QString& name, const QString& action, bool enabled, int priority,
                           const QString& roleRequired, const QString& departmentRequired, int minClearance,
                           const QString& resourceType, qint64 subjectId, qint64 resourceId)
{
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral("UPDATE policies SET name = :name, action = :action, enabled = :enabled, "
                             "priority = :priority, role_required = :role_required, "
                             "department_required = :department_required, min_clearance = :min_clearance, "
                             "resource_type = :resource_type, subject_id = :subject_id, "
                             "resource_id = :resource_id, updated_at = now() WHERE id = :id"));
    q.bindValue(QStringLiteral(":name"), name);
    q.bindValue(QStringLiteral(":action"), action);
    q.bindValue(QStringLiteral(":enabled"), enabled);
    q.bindValue(QStringLiteral(":priority"), priority);
    q.bindValue(QStringLiteral(":role_required"), roleRequired.isEmpty() ? QVariant() : roleRequired);
    q.bindValue(QStringLiteral(":department_required"), departmentRequired.isEmpty() ? QVariant() : departmentRequired);
    q.bindValue(QStringLiteral(":min_clearance"), minClearance < 0 ? QVariant() : minClearance);
    q.bindValue(QStringLiteral(":resource_type"), resourceType.isEmpty() ? QVariant() : resourceType);
    q.bindValue(QStringLiteral(":subject_id"), subjectId > 0 ? subjectId : QVariant());
    q.bindValue(QStringLiteral(":resource_id"), resourceId > 0 ? resourceId : QVariant());
    q.bindValue(QStringLiteral(":id"), id);
    return q.exec();
}

bool DbLayer::deletePolicy(qint64 id)
{
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral("DELETE FROM policies WHERE id = :id"));
    q.bindValue(QStringLiteral(":id"), id);
    return q.exec();
}

QVector<PolicyRecord> DbLayer::listPolicies(int page, int pageSize)
{
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral("SELECT id, name, enabled, action, role_required, department_required, "
                             "min_clearance, resource_type, subject_id, resource_id, priority, created_by "
                             "FROM policies ORDER BY priority DESC, id LIMIT :limit OFFSET :offset"));
    q.bindValue(QStringLiteral(":limit"), pageSize);
    q.bindValue(QStringLiteral(":offset"), (page - 1) * pageSize);
    q.exec();

    QVector<PolicyRecord> policies;
    while (q.next())
    {
        PolicyRecord p;
        p.id = q.value(0).toLongLong();
        p.name = q.value(1).toString();
        p.enabled = q.value(2).toBool();
        p.action = q.value(3).toString();
        p.role_required = q.value(4).toString();
        p.department_required = q.value(5).toString();
        p.min_clearance = q.value(6).isNull() ? -1 : q.value(6).toInt();
        p.resource_type = q.value(7).toString();
        p.subject_id = q.value(8).isNull() ? 0 : q.value(8).toLongLong();
        p.resource_id = q.value(9).isNull() ? 0 : q.value(9).toLongLong();
        p.priority = q.value(10).toInt();
        p.created_by = q.value(11).isNull() ? 0 : q.value(11).toLongLong();
        policies.append(p);
    }
    return policies;
}

int DbLayer::countPolicies()
{
    QSqlQuery q(m_db);
    q.exec(QStringLiteral("SELECT count(*) FROM policies"));
    return q.next() ? q.value(0).toInt() : 0;
}

QVector<PolicyRecord> DbLayer::findEnabledPolicies(const QString& action)
{
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral("SELECT id, name, enabled, action, role_required, department_required, "
                             "min_clearance, resource_type, subject_id, resource_id, priority, created_by "
                             "FROM policies WHERE enabled = TRUE AND (action = :action OR action = '*') "
                             "ORDER BY priority DESC"));
    q.bindValue(QStringLiteral(":action"), action);
    q.exec();

    QVector<PolicyRecord> policies;
    while (q.next())
    {
        PolicyRecord p;
        p.id = q.value(0).toLongLong();
        p.name = q.value(1).toString();
        p.enabled = q.value(2).toBool();
        p.action = q.value(3).toString();
        p.role_required = q.value(4).toString();
        p.department_required = q.value(5).toString();
        p.min_clearance = q.value(6).isNull() ? -1 : q.value(6).toInt();
        p.resource_type = q.value(7).toString();
        p.subject_id = q.value(8).isNull() ? 0 : q.value(8).toLongLong();
        p.resource_id = q.value(9).isNull() ? 0 : q.value(9).toLongLong();
        p.priority = q.value(10).toInt();
        p.created_by = q.value(11).isNull() ? 0 : q.value(11).toLongLong();
        policies.append(p);
    }
    return policies;
}

// ---------------------------------------------------------------------------
// Audit
// ---------------------------------------------------------------------------

void DbLayer::writeAuditLog(qint64 actorId, const QString& actorName, const QString& action, const QString& targetType,
                            qint64 targetId, const QString& result, const QString& details)
{
    QSqlQuery q(m_db);
    q.prepare(
        QStringLiteral("INSERT INTO audit_log (actor_id, actor_name, action, target_type, target_id, result, details) "
                       "VALUES (:actor_id, :actor_name, :action, :target_type, :target_id, :result, :details::jsonb)"));
    q.bindValue(QStringLiteral(":actor_id"), actorId > 0 ? actorId : QVariant());
    q.bindValue(QStringLiteral(":actor_name"), actorName);
    q.bindValue(QStringLiteral(":action"), action);
    q.bindValue(QStringLiteral(":target_type"), targetType);
    q.bindValue(QStringLiteral(":target_id"), targetId > 0 ? targetId : QVariant());
    q.bindValue(QStringLiteral(":result"), result);
    q.bindValue(QStringLiteral(":details"), details.isEmpty() ? QStringLiteral("{}") : details);
    q.exec();
}

QVector<AuditRecord> DbLayer::queryAuditLog(const AuditFilter& filter)
{
    QString sql = "SELECT id, ts, actor_id, actor_name, action, target_type, target_id, result, details "
                  "FROM audit_log WHERE 1=1";

    if (!filter.from.isEmpty())
    {
        sql += " AND ts >= :from";
    }
    if (!filter.to.isEmpty())
    {
        sql += " AND ts <= :to";
    }
    if (filter.actor_id > 0)
    {
        sql += " AND actor_id = :actor_id";
    }
    if (!filter.action.isEmpty())
    {
        sql += " AND action = :action";
    }

    sql += " ORDER BY ts DESC LIMIT :limit OFFSET :offset";

    QSqlQuery q(m_db);
    q.prepare(sql);

    if (!filter.from.isEmpty())
    {
        q.bindValue(QStringLiteral(":from"), filter.from);
    }
    if (!filter.to.isEmpty())
    {
        q.bindValue(QStringLiteral(":to"), filter.to);
    }
    if (filter.actor_id > 0)
    {
        q.bindValue(QStringLiteral(":actor_id"), filter.actor_id);
    }
    if (!filter.action.isEmpty())
    {
        q.bindValue(QStringLiteral(":action"), filter.action);
    }
    q.bindValue(QStringLiteral(":limit"), filter.page_size);
    q.bindValue(QStringLiteral(":offset"), (filter.page - 1) * filter.page_size);
    q.exec();

    QVector<AuditRecord> records;
    while (q.next())
    {
        AuditRecord r;
        r.id = q.value(0).toLongLong();
        r.ts = q.value(1).toString();
        r.actor_id = q.value(2).isNull() ? 0 : q.value(2).toLongLong();
        r.actor_name = q.value(3).toString();
        r.action = q.value(4).toString();
        r.target_type = q.value(5).toString();
        r.target_id = q.value(6).isNull() ? 0 : q.value(6).toLongLong();
        r.result = q.value(7).toString();
        r.details = q.value(8).toString();
        records.append(r);
    }
    return records;
}

int DbLayer::countAuditLog(const AuditFilter& filter)
{
    QString sql = "SELECT count(*) FROM audit_log WHERE 1=1";

    if (!filter.from.isEmpty())
    {
        sql += " AND ts >= :from";
    }
    if (!filter.to.isEmpty())
    {
        sql += " AND ts <= :to";
    }
    if (filter.actor_id > 0)
    {
        sql += " AND actor_id = :actor_id";
    }
    if (!filter.action.isEmpty())
    {
        sql += " AND action = :action";
    }

    QSqlQuery q(m_db);
    q.prepare(sql);

    if (!filter.from.isEmpty())
    {
        q.bindValue(QStringLiteral(":from"), filter.from);
    }
    if (!filter.to.isEmpty())
    {
        q.bindValue(QStringLiteral(":to"), filter.to);
    }
    if (filter.actor_id > 0)
    {
        q.bindValue(QStringLiteral(":actor_id"), filter.actor_id);
    }
    if (!filter.action.isEmpty())
    {
        q.bindValue(QStringLiteral(":action"), filter.action);
    }
    q.exec();
    return q.next() ? q.value(0).toInt() : 0;
}

} // namespace db
