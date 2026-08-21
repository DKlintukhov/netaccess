/**
 * @file config.h
 * @brief Server configuration for netaccess.
 *
 * Loads a JSON configuration file and provides a ServerConfig struct
 * with sensible defaults per docs/architecture.md §6.
 */

#pragma once

#include <QString>

namespace server
{

/**
 * @brief Server runtime configuration.
 */
struct ServerConfig
{
    // Network
    QString listen_host = QStringLiteral("0.0.0.0");
    quint16 listen_port = 9988;

    // TLS
    QString tls_cert = QStringLiteral("certs/server.crt");
    QString tls_key = QStringLiteral("certs/server.key");

    // PostgreSQL
    QString db_host = QStringLiteral("localhost");
    quint16 db_port = 5432;
    QString db_name = QStringLiteral("netaccess");
    QString db_user = QStringLiteral("netaccess");
    QString db_password;

    // Sessions
    int session_lifetime_h = 8; ///< Token lifetime in hours.
    int max_sessions = 100;     ///< Maximum concurrent sessions.

    // Brute-force protection
    int max_failed_attempts = 5;
    int lockout_minutes = 15;
};

/**
 * @brief Loads the server configuration from a JSON file.
 *
 * Missing keys keep their defaults.  Returns a default-constructed
 * ServerConfig if the file does not exist or is malformed.
 *
 * @param[in] path Path to the JSON configuration file.
 * @return Parsed configuration.
 */
ServerConfig loadConfig(const QString& path);

/**
 * @brief Saves the server configuration to a JSON file.
 *
 * @param[in] config Configuration to save.
 * @param[in] path   Destination file path.
 * @return true on success.
 */
bool saveConfig(const ServerConfig& config, const QString& path);

} // namespace server
