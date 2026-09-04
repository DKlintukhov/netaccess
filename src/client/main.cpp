#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QTranslator>
#include <QUrl>

#include <client/api_client.h>
#include <client/session_state.h>
#include <netaccess/common.h>

int main(int argc, char* argv[])
{
    QGuiApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("netaccess_client"));
    app.setApplicationVersion(QString::fromUtf8(netaccess::versionString()));

    // Load a translation matched to the system locale. The packaged catalogs
    // use the ISO 639-1 language code only (netaccess_ru, netaccess_en), so
    // fall back from the full locale ("ru_RU") to the bare language code.
    const QString fullLocale = QLocale::system().name(); // e.g. "ru_RU"
    const QString langCode = fullLocale.section(QLatin1Char('_'), 0, 0);
    const QString prefix = QStringLiteral(":/translations/netaccess_");
    QTranslator translator;
    bool loaded = translator.load(prefix + fullLocale);
    if (!loaded && langCode != fullLocale)
    {
        loaded = translator.load(prefix + langCode);
    }
    if (loaded)
    {
        app.installTranslator(&translator);
    }

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
