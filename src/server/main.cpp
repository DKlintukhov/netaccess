#include <QCoreApplication>
#include <QTextStream>

#include <netaccess/common.h>

#include <server/authenticator.h>
#include <server/config.h>
#include <server/db_layer.h>
#include <server/handlers.h>
#include <server/listener.h>
#include <server/policy_engine.h>
#include <server/session_manager.h>

int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("netaccess_server"));
    app.setApplicationVersion(QString::fromUtf8(netaccess::versionString()));

    QTextStream out(stdout);
    out << "netaccess_server " << app.applicationVersion() << Qt::endl;

    // Load configuration.
    const QString configPath = (argc > 1) ? QString::fromLocal8Bit(argv[1]) : QStringLiteral("config.json");
    auto cfg = server::loadConfig(configPath);
    out << "Config loaded from " << configPath << Qt::endl;

    // Connect to database.
    db::DbLayer db;
    if (!db.open(cfg))
    {
        out << "ERROR: Failed to connect to PostgreSQL" << Qt::endl;
        return 1;
    }
    out << "Connected to PostgreSQL" << Qt::endl;

    // Apply migrations.
    if (!db.applyMigrations(QStringLiteral("sql/V001__schema.sql"), QStringLiteral("sql/V002__seed.sql")))
    {
        out << "WARNING: Migration failed (tables may already exist)" << Qt::endl;
    }

    // Build components.
    auth::Authenticator auth(db, cfg);
    session::SessionManager sessions(db, cfg);
    policy::PolicyEngine policy(db);
    handlers::Handler handler(db, auth, sessions, policy);

    // Start listener.
    server::Listener listener(cfg, handler);
    QObject::connect(&listener, &server::Listener::errorOccurred,
                     [&out](const QString& msg) { out << "ERROR: " << msg << Qt::endl; });

    if (!listener.start())
    {
        return 1;
    }

    out << "Listening on " << listener.listenAddress().toString() << ":" << listener.listenPort() << Qt::endl;
    out << "Server ready." << Qt::endl;

    return app.exec();
}
