#include <client/api_client.h>

namespace client
{

ApiClient::ApiClient(QObject* parent) : QObject(parent), m_socket(new QSslSocket(this))
{
    connect(m_socket, &QSslSocket::connected, this, &ApiClient::onConnected);
    connect(m_socket, &QSslSocket::disconnected, this, &ApiClient::onDisconnected);
    connect(m_socket, &QSslSocket::readyRead, this, &ApiClient::onReadyRead);
    connect(m_socket, QOverload<const QList<QSslError>&>::of(&QSslSocket::sslErrors), this, &ApiClient::onSslErrors);
}

ApiClient::~ApiClient()
{
    disconnectFromServer();
}

bool ApiClient::isConnected() const
{
    return m_socket->state() == QAbstractSocket::ConnectedState;
}

bool ApiClient::isBusy() const
{
    return m_busy;
}

void ApiClient::connectToServer(const QString& host, quint16 port, const QString& pinSha1)
{
    if (isConnected())
    {
        return;
    }

    m_pinSha1 = pinSha1;
    m_socket->connectToHost(host, port);
}

void ApiClient::disconnectFromServer()
{
    if (m_socket->state() != QAbstractSocket::UnconnectedState)
    {
        m_socket->disconnectFromHost();
    }
}

void ApiClient::sendRequest(const QString& op, int reqId, const QJsonObject& data, const QString& token)
{
    if (!isConnected())
    {
        return;
    }

    protocol::Request req;
    req.op = protocol::opFromString(op).value_or(protocol::Op::AUTH);
    req.req_id = reqId;
    req.data = data;
    req.token = token;

    m_socket->write(protocol::frame(req.toJson()));
    m_socket->flush();

    m_busy = true;
    emit busyChanged();
}

// ---------------------------------------------------------------------------
// Private slots
// ---------------------------------------------------------------------------

void ApiClient::onConnected()
{
    // Start TLS if not already encrypted.
    if (!m_socket->isEncrypted())
    {
        m_socket->startClientEncryption();
    }
    else
    {
        emit connectedToServer();
        emit connectedChanged();
    }
}

void ApiClient::onDisconnected()
{
    m_busy = false;
    emit busyChanged();
    emit disconnectedFromServer();
    emit connectedChanged();
}

void ApiClient::onReadyRead()
{
    processResponse();
}

void ApiClient::onSslErrors(const QList<QSslError>& errors)
{
    // Filter out self-signed certificate errors (expected in dev).
    QList<QSslError> ignorable;
    for (const auto& err : errors)
    {
        if (err.error() == QSslError::SelfSignedCertificate || err.error() == QSslError::CertificateUntrusted ||
            err.error() == QSslError::HostNameMismatch)
        {
            ignorable.append(err);
        }
        else
        {
            emit sslError(err.errorString());
        }
    }
    m_socket->ignoreSslErrors(ignorable);

    // If connected after ignoring errors, emit connected.
    if (m_socket->isEncrypted())
    {
        emit connectedToServer();
        emit connectedChanged();
    }
}

void ApiClient::processResponse()
{
    auto frame = protocol::readFrame(m_socket);
    if (!frame)
    {
        return;
    }

    auto response = protocol::Response::fromJson(*frame);
    if (!response)
    {
        return;
    }

    QJsonObject respObj;
    respObj["op"] = QString(protocol::opToString(response->op));
    respObj["req_id"] = response->req_id;
    respObj["status"] = QString(protocol::statusToString(response->status));
    respObj["code"] = QString(protocol::resultCodeToString(response->code));
    respObj["message"] = response->message;
    respObj["data"] = response->data;

    m_busy = false;
    emit busyChanged();
    emit responseReceived(response->req_id, respObj);
}

} // namespace client
