#include <server/config.h>

#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>

namespace server
{

ServerConfig loadConfig(const QString& path)
{
    ServerConfig cfg;

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly))
    {
        return cfg;
    }

    QJsonParseError parseError{};
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !doc.isObject())
    {
        return cfg;
    }

    const QJsonObject obj = doc.object();

    // Network
    cfg.listen_host = obj["listen_host"].toString(cfg.listen_host);
    cfg.listen_port = static_cast<quint16>(obj["listen_port"].toInt(cfg.listen_port));

    // TLS
    cfg.tls_cert = obj["tls_cert"].toString(cfg.tls_cert);
    cfg.tls_key = obj["tls_key"].toString(cfg.tls_key);

    // PostgreSQL
    cfg.db_host = obj["db_host"].toString(cfg.db_host);
    cfg.db_port = static_cast<quint16>(obj["db_port"].toInt(cfg.db_port));
    cfg.db_name = obj["db_name"].toString(cfg.db_name);
    cfg.db_user = obj["db_user"].toString(cfg.db_user);
    cfg.db_password = obj["db_password"].toString(cfg.db_password);

    // Sessions
    cfg.session_lifetime_h = obj["session_lifetime_h"].toInt(cfg.session_lifetime_h);
    cfg.max_sessions = obj["max_sessions"].toInt(cfg.max_sessions);

    // Brute-force
    cfg.max_failed_attempts = obj["max_failed_attempts"].toInt(cfg.max_failed_attempts);
    cfg.lockout_minutes = obj["lockout_minutes"].toInt(cfg.lockout_minutes);

    return cfg;
}

bool saveConfig(const ServerConfig& config, const QString& path)
{
    QJsonObject obj;

    obj["listen_host"] = config.listen_host;
    obj["listen_port"] = config.listen_port;

    obj["tls_cert"] = config.tls_cert;
    obj["tls_key"] = config.tls_key;

    obj["db_host"] = config.db_host;
    obj["db_port"] = config.db_port;
    obj["db_name"] = config.db_name;
    obj["db_user"] = config.db_user;
    obj["db_password"] = config.db_password;

    obj["session_lifetime_h"] = config.session_lifetime_h;
    obj["max_sessions"] = config.max_sessions;

    obj["max_failed_attempts"] = config.max_failed_attempts;
    obj["lockout_minutes"] = config.lockout_minutes;

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly))
    {
        return false;
    }

    file.write(QJsonDocument(obj).toJson(QJsonDocument::Compact));
    return true;
}

} // namespace server
