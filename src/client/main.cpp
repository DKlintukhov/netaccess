#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QUrl>

#include <client/api_client.h>
#include <client/session_state.h>
#include <netaccess/common.h>

int main(int argc, char* argv[])
{
    QGuiApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("netaccess_client"));
    app.setApplicationVersion(QString::fromUtf8(netaccess::versionString()));

    // Register QML types.
    qmlRegisterType<client::ApiClient>("NetAccess", 1, 0, "ApiClient");
    qmlRegisterType<client::SessionState>("NetAccess", 1, 0, "SessionState");

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty("netaccessVersion", QString::fromUtf8(netaccess::versionString()));

    engine.load(QUrl(QStringLiteral("qrc:/qml/Main.qml")));
    if (engine.rootObjects().isEmpty())
    {
        return -1;
    }

    return app.exec();
}
