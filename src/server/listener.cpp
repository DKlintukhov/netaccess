#include <server/listener.h>

#include <QFile>
#include <QJsonDocument>
#include <QTextStream>

#include <netaccess/protocol.h>

namespace server
{

Listener::Listener(const ServerConfig& cfg, handlers::Handler& handler, QObject* parent)
    : QObject(parent), m_cfg(cfg), m_handler(handler), m_server(new QTcpServer(this))
{
    connect(m_server, &QTcpServer::newConnection, this, &Listener::onNewConnection);
}

Listener::~Listener()
{
    stop();
}

bool Listener::start()
{
    if (!loadTlsCertificate())
    {
        emit errorOccurred(QStringLiteral("Failed to load TLS certificate"));
        return false;
    }

    const QHostAddress addr(m_cfg.listen_host);
    if (!m_server->listen(addr, m_cfg.listen_port))
    {
        emit errorOccurred(QStringLiteral("Failed to listen on %1:%2: %3")
                               .arg(m_cfg.listen_host)
                               .arg(m_cfg.listen_port)
                               .arg(m_server->errorString()));
        return false;
    }

    return true;
}

void Listener::stop()
{
    m_server->close();
    m_server->deleteLater();
    m_server = nullptr;
}

QHostAddress Listener::listenAddress() const
{
    return m_server ? m_server->serverAddress() : QHostAddress();
}

quint16 Listener::listenPort() const
{
    return m_server ? m_server->serverPort() : 0;
}

// ---------------------------------------------------------------------------
// TLS
// ---------------------------------------------------------------------------

bool Listener::loadTlsCertificate()
{
    QFile certFile(m_cfg.tls_cert);
    if (!certFile.open(QIODevice::ReadOnly))
    {
        return false;
    }
    m_cert = QSslCertificate(certFile.readAll(), QSsl::Pem);

    QFile keyFile(m_cfg.tls_key);
    if (!keyFile.open(QIODevice::ReadOnly))
    {
        return false;
    }
    m_key = QSslKey(keyFile.readAll(), QSsl::Rsa);

    return !m_cert.isNull() && !m_key.isNull();
}

// ---------------------------------------------------------------------------
// Connection handling
// ---------------------------------------------------------------------------

void Listener::onNewConnection()
{
    while (m_server->hasPendingConnections())
    {
        QTcpSocket* tcpSocket = m_server->nextPendingConnection();

        auto* sslSocket = new QSslSocket(this);
        if (!sslSocket->setSocketDescriptor(tcpSocket->socketDescriptor()))
        {
            delete sslSocket;
            tcpSocket->disconnectFromHost();
            continue;
        }
        tcpSocket->setSocketDescriptor(-1); // release from tcpSocket
        delete tcpSocket;

        sslSocket->setLocalCertificate(m_cert);
        sslSocket->setPrivateKey(m_key);
        sslSocket->setProtocol(QSsl::TlsV1_3);

        connect(sslSocket, &QSslSocket::readyRead, this, &Listener::onReadyRead);
        connect(sslSocket, &QSslSocket::disconnected, this, &Listener::onDisconnected);
        connect(sslSocket, QOverload<const QList<QSslError>&>::of(&QSslSocket::sslErrors), this,
                &Listener::onSslErrors);

        sslSocket->startServerEncryption();
    }
}

void Listener::onReadyRead()
{
    auto* socket = qobject_cast<QSslSocket*>(sender());
    if (!socket || socket->bytesAvailable() == 0)
    {
        return;
    }

    processRequest(socket);
}

void Listener::onDisconnected()
{
    auto* socket = qobject_cast<QSslSocket*>(sender());
    if (socket)
    {
        socket->deleteLater();
    }
}

void Listener::onSslErrors(const QList<QSslError>& errors)
{
    for (const auto& err : errors)
    {
        Q_UNUSED(err)
        // Ignore self-signed certificate errors during development.
    }
}

// ---------------------------------------------------------------------------
// Request processing
// ---------------------------------------------------------------------------

void Listener::processRequest(QSslSocket* socket)
{
    auto frame = protocol::readFrame(socket);
    if (!frame)
    {
        return;
    }

    auto request = protocol::Request::fromJson(*frame);
    if (!request)
    {
        protocol::Response err;
        err.op = protocol::Op::AUTH;
        err.status = protocol::Status::error;
        err.code = protocol::ResultCode::VALIDATION_ERROR;
        err.message = QStringLiteral("Malformed request");
        socket->write(protocol::frame(err.toJson()));
        socket->flush();
        return;
    }

    auto response = m_handler.handle(*request);
    socket->write(protocol::frame(response.toJson()));
    socket->flush();
}

} // namespace server
