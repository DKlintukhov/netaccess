/**
 * @file api_client.h
 * @brief Network client for the netaccess protocol.
 *
 * Connects to the server via QSslSocket (TLS), sends framed JSON
 * requests, and receives framed JSON responses.
 */

#pragma once

#include <QObject>
#include <QSslCertificate>
#include <QSslSocket>

#include <netaccess/protocol.h>

namespace client
{

/**
 * @brief Network client for the netaccess server.
 */
class ApiClient : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool connected READ isConnected NOTIFY connectedChanged)
    Q_PROPERTY(bool busy READ isBusy NOTIFY busyChanged)

public:
    explicit ApiClient(QObject* parent = nullptr);
    ~ApiClient() override;

    bool isConnected() const;
    bool isBusy() const;

    /// Connects to the server.  pinSha1 is the expected certificate fingerprint.
    Q_INVOKABLE void connectToServer(const QString& host, quint16 port, const QString& pinSha1 = {});

    /// Disconnects from the server.
    Q_INVOKABLE void disconnectFromServer();

    /// Sends a request and returns the response asynchronously.
    Q_INVOKABLE void sendRequest(const QString& op, int reqId, const QJsonObject& data = {}, const QString& token = {});

signals:
    void connectedChanged();
    void busyChanged();
    void connectedToServer();
    void disconnectedFromServer();
    void connectionError(const QString& message);
    void responseReceived(int reqId, const QJsonObject& response);
    void sslError(const QString& message);

private slots:
    void onConnected();
    void onDisconnected();
    void onReadyRead();
    void onSslErrors(const QList<QSslError>& errors);

private:
    void processResponse();

    QSslSocket* m_socket = nullptr;
    QString m_pinSha1;
    bool m_busy = false;
};

} // namespace client
