/**
 * @file protocol.h
 * @brief Client-server protocol for netaccess.
 *
 * Defines request/response DTOs, operation and result codes, and
 * length-prefixed JSON framing (u32 big-endian + payload).  Both the
 * server and the client link against this module.
 *
 * Wire format per docs/protocol.md:
 *   +----------------+-----------------------------+
 *   | length (u32 BE) | payload (length bytes JSON) |
 *   +----------------+-----------------------------+
 */

#pragma once

#include <QJsonObject>
#include <QString>
#include <optional>

namespace protocol
{

/// Protocol version.
inline constexpr int kVersion = 1;

/// Maximum payload size in bytes (16 MiB, per docs/protocol.md §7).
inline constexpr quint32 kMaxPayloadSize = 16 * 1024 * 1024;

// ---------------------------------------------------------------------------
// Operations (client -> server).
// ---------------------------------------------------------------------------

/**
 * @brief Operations supported by the protocol.
 */
enum class Op : quint8
{
    AUTH,            ///< Authenticate, obtain a session token.
    LOGOUT,          ///< Terminate the current session.
    ME,              ///< Current user info and attributes.
    RESOURCE_LIST,   ///< List resources (filtered).
    RESOURCE_GET,    ///< Single resource card.
    RESOURCE_CREATE, ///< Add a resource to the catalogue (admin).
    RESOURCE_UPDATE, ///< Modify a resource (admin).
    RESOURCE_DELETE, ///< Remove a resource (admin).
    POLICY_LIST,     ///< List ABAC policies.
    POLICY_CREATE,   ///< Create a policy (admin).
    POLICY_UPDATE,   ///< Modify a policy (admin).
    POLICY_DELETE,   ///< Remove a policy (admin).
    USER_LIST,       ///< List users (admin).
    USER_CREATE,     ///< Create a user (admin).
    USER_UPDATE,     ///< Modify a user (admin).
    USER_DELETE,     ///< Delete a user (admin).
    ACCESS_CHECK,    ///< Test access to a resource.
    GRANT_ACCESS,    ///< Grant access (create policy).
    REVOKE_ACCESS,   ///< Revoke access (disable/delete policy).
    AUDIT_QUERY,     ///< Query the audit log.
};

/// Converts an operation to its wire code.
const char* opToString(Op op);

/// Converts a wire code to an operation; returns std::nullopt on unknown code.
std::optional<Op> opFromString(const QString& s);

// ---------------------------------------------------------------------------
// Result status (top-level) and detailed result code.
// ---------------------------------------------------------------------------

/**
 * @brief Top-level status of a response.
 */
enum class Status : quint8
{
    ok,     ///< Operation succeeded.
    denied, ///< Access denied (ABAC).
    error,  ///< Server-side error or validation failure.
};

/// Converts a status to its wire value ("ok", "denied", "error").
const char* statusToString(Status s);

/// Converts a wire value to a status; returns std::nullopt on unknown value.
std::optional<Status> statusFromString(const QString& s);

/**
 * @brief Detailed result code (the "code" field of a response).
 */
enum class ResultCode : quint8
{
    OK,
    AUTH_DENIED,
    AUTH_FAILED_TOO_MANY,
    ACCOUNT_LOCKED,
    ACCOUNT_INACTIVE,
    TOKEN_INVALID,
    TOKEN_EXPIRED,
    ACCESS_DENIED,
    RESOURCE_NOT_FOUND,
    VALIDATION_ERROR,
    UNSUPPORTED_OP,
    SERVER_BUSY,
    INTERNAL_ERROR,
};

/// Converts a result code to its wire string.
const char* resultCodeToString(ResultCode rc);

/// Converts a wire string to a result code; returns std::nullopt on unknown code.
std::optional<ResultCode> resultCodeFromString(const QString& s);

// ---------------------------------------------------------------------------
// Request / Response DTOs.
// ---------------------------------------------------------------------------

/**
 * @brief Request sent by the client.
 */
struct Request
{
    int version = kVersion;
    Op op = Op::AUTH;
    int req_id = 0;
    QString token;    ///< Session token (empty for AUTH).
    QJsonObject data; ///< Operation-specific payload.

    /// Serialises the request to JSON.
    QJsonObject toJson() const;

    /// Deserialises a request from JSON; returns std::nullopt on malformed input.
    static std::optional<Request> fromJson(const QJsonObject& obj);
};

/**
 * @brief Response sent by the server.
 */
struct Response
{
    int version = kVersion;
    Op op = Op::AUTH;
    int req_id = 0;
    Status status = Status::ok;
    ResultCode code = ResultCode::OK;
    QString message;  ///< Human-readable explanation.
    QJsonObject data; ///< Operation-specific payload.

    /// Serialises the response to JSON.
    QJsonObject toJson() const;

    /// Deserialises a response from JSON; returns std::nullopt on malformed input.
    static std::optional<Response> fromJson(const QJsonObject& obj);
};

// ---------------------------------------------------------------------------
// Framing: u32 BE length prefix + JSON payload.
// ---------------------------------------------------------------------------

/**
 * @brief Wraps a JSON object into a length-prefixed frame.
 *
 * @param[in] json The object to serialise.
 * @return QByteArray ready to write to a QTcpSocket.
 */
QByteArray frame(const QJsonObject& json);

/**
 * @brief Reads one frame from a QIODevice and returns the parsed JSON.
 *
 * @param[in] device The device to read from (e.g. QTcpSocket).
 * @return The parsed JSON object, or std::nullopt if not enough data or
 *         the frame is malformed / exceeds kMaxPayloadSize.
 */
std::optional<QJsonObject> readFrame(QIODevice* device);

} // namespace protocol
