#include <netaccess/protocol.h>

#include <QDataStream>
#include <QIODevice>
#include <QJsonDocument>

namespace protocol
{

// ---------------------------------------------------------------------------
// Op
// ---------------------------------------------------------------------------

const char* opToString(Op op)
{
    switch (op)
    {
    case Op::AUTH:
        return "AUTH";
    case Op::LOGOUT:
        return "LOGOUT";
    case Op::ME:
        return "ME";
    case Op::RESOURCE_LIST:
        return "RESOURCE_LIST";
    case Op::RESOURCE_GET:
        return "RESOURCE_GET";
    case Op::RESOURCE_CREATE:
        return "RESOURCE_CREATE";
    case Op::RESOURCE_UPDATE:
        return "RESOURCE_UPDATE";
    case Op::RESOURCE_DELETE:
        return "RESOURCE_DELETE";
    case Op::POLICY_LIST:
        return "POLICY_LIST";
    case Op::POLICY_CREATE:
        return "POLICY_CREATE";
    case Op::POLICY_UPDATE:
        return "POLICY_UPDATE";
    case Op::POLICY_DELETE:
        return "POLICY_DELETE";
    case Op::USER_LIST:
        return "USER_LIST";
    case Op::USER_CREATE:
        return "USER_CREATE";
    case Op::USER_UPDATE:
        return "USER_UPDATE";
    case Op::USER_DELETE:
        return "USER_DELETE";
    case Op::ACCESS_CHECK:
        return "ACCESS_CHECK";
    case Op::GRANT_ACCESS:
        return "GRANT_ACCESS";
    case Op::REVOKE_ACCESS:
        return "REVOKE_ACCESS";
    case Op::AUDIT_QUERY:
        return "AUDIT_QUERY";
    }
    return nullptr;
}

std::optional<Op> opFromString(const QString& s)
{
    static const QHash<QString, Op> map = {
        {"AUTH", Op::AUTH},
        {"LOGOUT", Op::LOGOUT},
        {"ME", Op::ME},
        {"RESOURCE_LIST", Op::RESOURCE_LIST},
        {"RESOURCE_GET", Op::RESOURCE_GET},
        {"RESOURCE_CREATE", Op::RESOURCE_CREATE},
        {"RESOURCE_UPDATE", Op::RESOURCE_UPDATE},
        {"RESOURCE_DELETE", Op::RESOURCE_DELETE},
        {"POLICY_LIST", Op::POLICY_LIST},
        {"POLICY_CREATE", Op::POLICY_CREATE},
        {"POLICY_UPDATE", Op::POLICY_UPDATE},
        {"POLICY_DELETE", Op::POLICY_DELETE},
        {"USER_LIST", Op::USER_LIST},
        {"USER_CREATE", Op::USER_CREATE},
        {"USER_UPDATE", Op::USER_UPDATE},
        {"USER_DELETE", Op::USER_DELETE},
        {"ACCESS_CHECK", Op::ACCESS_CHECK},
        {"GRANT_ACCESS", Op::GRANT_ACCESS},
        {"REVOKE_ACCESS", Op::REVOKE_ACCESS},
        {"AUDIT_QUERY", Op::AUDIT_QUERY},
    };
    auto it = map.find(s);
    return (it != map.end()) ? std::optional(*it) : std::nullopt;
}

// ---------------------------------------------------------------------------
// Status
// ---------------------------------------------------------------------------

const char* statusToString(Status s)
{
    switch (s)
    {
    case Status::ok:
        return "ok";
    case Status::denied:
        return "denied";
    case Status::error:
        return "error";
    }
    return nullptr;
}

std::optional<Status> statusFromString(const QString& s)
{
    if (s == "ok")
        return Status::ok;
    if (s == "denied")
        return Status::denied;
    if (s == "error")
        return Status::error;
    return std::nullopt;
}

// ---------------------------------------------------------------------------
// ResultCode
// ---------------------------------------------------------------------------

const char* resultCodeToString(ResultCode rc)
{
    switch (rc)
    {
    case ResultCode::OK:
        return "OK";
    case ResultCode::AUTH_DENIED:
        return "AUTH_DENIED";
    case ResultCode::AUTH_FAILED_TOO_MANY:
        return "AUTH_FAILED_TOO_MANY";
    case ResultCode::ACCOUNT_LOCKED:
        return "ACCOUNT_LOCKED";
    case ResultCode::ACCOUNT_INACTIVE:
        return "ACCOUNT_INACTIVE";
    case ResultCode::TOKEN_INVALID:
        return "TOKEN_INVALID";
    case ResultCode::TOKEN_EXPIRED:
        return "TOKEN_EXPIRED";
    case ResultCode::ACCESS_DENIED:
        return "ACCESS_DENIED";
    case ResultCode::RESOURCE_NOT_FOUND:
        return "RESOURCE_NOT_FOUND";
    case ResultCode::VALIDATION_ERROR:
        return "VALIDATION_ERROR";
    case ResultCode::UNSUPPORTED_OP:
        return "UNSUPPORTED_OP";
    case ResultCode::SERVER_BUSY:
        return "SERVER_BUSY";
    case ResultCode::INTERNAL_ERROR:
        return "INTERNAL_ERROR";
    }
    return nullptr;
}

std::optional<ResultCode> resultCodeFromString(const QString& s)
{
    static const QHash<QString, ResultCode> map = {
        {"OK", ResultCode::OK},
        {"AUTH_DENIED", ResultCode::AUTH_DENIED},
        {"AUTH_FAILED_TOO_MANY", ResultCode::AUTH_FAILED_TOO_MANY},
        {"ACCOUNT_LOCKED", ResultCode::ACCOUNT_LOCKED},
        {"ACCOUNT_INACTIVE", ResultCode::ACCOUNT_INACTIVE},
        {"TOKEN_INVALID", ResultCode::TOKEN_INVALID},
        {"TOKEN_EXPIRED", ResultCode::TOKEN_EXPIRED},
        {"ACCESS_DENIED", ResultCode::ACCESS_DENIED},
        {"RESOURCE_NOT_FOUND", ResultCode::RESOURCE_NOT_FOUND},
        {"VALIDATION_ERROR", ResultCode::VALIDATION_ERROR},
        {"UNSUPPORTED_OP", ResultCode::UNSUPPORTED_OP},
        {"SERVER_BUSY", ResultCode::SERVER_BUSY},
        {"INTERNAL_ERROR", ResultCode::INTERNAL_ERROR},
    };
    auto it = map.find(s);
    return (it != map.end()) ? std::optional(*it) : std::nullopt;
}

// ---------------------------------------------------------------------------
// Request
// ---------------------------------------------------------------------------

QJsonObject Request::toJson() const
{
    QJsonObject obj;
    obj["version"] = version;
    obj["op"] = QString(opToString(op));
    obj["req_id"] = req_id;
    if (!token.isEmpty())
    {
        obj["token"] = token;
    }
    obj["data"] = data;
    return obj;
}

std::optional<Request> Request::fromJson(const QJsonObject& obj)
{
    if (!obj.contains("op") || !obj.contains("req_id"))
    {
        return std::nullopt;
    }

    auto op = opFromString(obj["op"].toString());
    if (!op)
    {
        return std::nullopt;
    }

    Request req;
    req.version = obj["version"].toInt(kVersion);
    req.op = *op;
    req.req_id = obj["req_id"].toInt();
    req.token = obj["token"].toString();
    req.data = obj["data"].toObject();
    return req;
}

// ---------------------------------------------------------------------------
// Response
// ---------------------------------------------------------------------------

QJsonObject Response::toJson() const
{
    QJsonObject obj;
    obj["version"] = version;
    obj["op"] = QString(opToString(op));
    obj["req_id"] = req_id;
    obj["status"] = QString(statusToString(status));
    obj["code"] = QString(resultCodeToString(code));
    obj["message"] = message;
    obj["data"] = data;
    return obj;
}

std::optional<Response> Response::fromJson(const QJsonObject& obj)
{
    if (!obj.contains("op") || !obj.contains("status") || !obj.contains("code"))
    {
        return std::nullopt;
    }

    auto op = opFromString(obj["op"].toString());
    if (!op)
    {
        return std::nullopt;
    }

    auto st = statusFromString(obj["status"].toString());
    if (!st)
    {
        return std::nullopt;
    }

    auto rc = resultCodeFromString(obj["code"].toString());
    if (!rc)
    {
        return std::nullopt;
    }

    Response resp;
    resp.version = obj["version"].toInt(kVersion);
    resp.op = *op;
    resp.req_id = obj["req_id"].toInt();
    resp.status = *st;
    resp.code = *rc;
    resp.message = obj["message"].toString();
    resp.data = obj["data"].toObject();
    return resp;
}

// ---------------------------------------------------------------------------
// Framing
// ---------------------------------------------------------------------------

QByteArray frame(const QJsonObject& json)
{
    QByteArray payload = QJsonDocument(json).toJson(QJsonDocument::Compact);

    QByteArray header;
    QDataStream stream(&header, QIODevice::WriteOnly);
    stream.setByteOrder(QDataStream::BigEndian);
    stream << static_cast<quint32>(payload.size());

    return header + payload;
}

std::optional<QJsonObject> readFrame(QIODevice* device)
{
    if (device->bytesAvailable() < 4)
    {
        return std::nullopt;
    }

    // Peek at the 4-byte header to determine payload length.
    QByteArray header = device->peek(4);
    QDataStream stream(header);
    stream.setByteOrder(QDataStream::BigEndian);
    quint32 len = 0;
    stream >> len;

    if (len > kMaxPayloadSize)
    {
        return std::nullopt;
    }

    // Wait until the full payload has arrived.
    if (device->bytesAvailable() < static_cast<qint64>(4 + len))
    {
        return std::nullopt;
    }

    device->read(4); // consume header
    QByteArray payload = device->read(len);

    QJsonParseError parseError{};
    QJsonDocument doc = QJsonDocument::fromJson(payload, &parseError);
    if (parseError.error != QJsonParseError::NoError || !doc.isObject())
    {
        return std::nullopt;
    }

    return doc.object();
}

} // namespace protocol
