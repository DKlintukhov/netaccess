/**
 * @file listener.h
 * @brief TCP+TLS listener for the netaccess server.
 *
 * Accepts incoming connections, performs TLS handshake, reads framed
 * JSON requests, dispatches them to the PEP, and writes framed
 * responses back.
 */

#pragma once

#include <QObject>
#include <QSslCertificate>
#include <QSslKey>
#include <QSslSocket>
#include <QTcpServer>

#include <server/pep.h>

namespace server
{

/**
 * @brief TCP+TLS listener.
 */
class Listener : public QObject
{
    Q_OBJECT

public:
    Listener(const ServerConfig& cfg, pep::PEP& pep, QObject* parent = nullptr);
    ~Listener() override;

    bool start();
    void stop();

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
    pep::PEP& m_pep;
    QTcpServer* m_server = nullptr;
    QSslCertificate m_cert;
    QSslKey m_key;
};

} // namespace server
