/**
 * @file listener.h
 * @brief TCP+TLS listener for the netaccept server.
 *
 * Accepts incoming connections, performs TLS handshake, reads framed
 * JSON requests, dispatches them to the Handler, and writes framed
 * responses back.
 */

#pragma once

#include <QObject>
#include <QSslCertificate>
#include <QSslKey>
#include <QSslSocket>
#include <QTcpServer>

#include <server/handlers.h>

namespace server
{

/**
 * @brief TCP+TLS listener.
 */
class Listener : public QObject
{
    Q_OBJECT

public:
    Listener(const ServerConfig& cfg, handlers::Handler& handler, QObject* parent = nullptr);
    ~Listener() override;

    /// Starts listening.  Returns false on error.
    bool start();

    /// Stops listening and closes all connections.
    void stop();

    /// Returns the address/port actually bound.
    QHostAddress listenAddress() const;
    quint16 listenPort() const;

signals:
    void errorOccurred(const QString& message);

private slots:
    void onNewConnection();
    void onReadyRead();
    void onDisconnected();
    void onSslErrors(const QList<QSslError>& errors);

private:
    bool loadTlsCertificate();
    void processRequest(QSslSocket* socket);

    ServerConfig m_cfg;
    handlers::Handler& m_handler;
    QTcpServer* m_server = nullptr;
    QSslCertificate m_cert;
    QSslKey m_key;
};

} // namespace server
