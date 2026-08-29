#include <QCoreApplication>
#include <QTextStream>

#include <netaccess/common.h>

#include <server/authenticator.h>
#include <server/config.h>
#include <server/listener.h>
#include <server/pap.h>
#include <server/pdp.h>
#include <server/pep.h>
#include <server/pip.h>
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

    // Connect to database (PIP).
    pip::PIP pip;
    if (!pip.open(cfg))
    {
        out << "ERROR: Failed to connect to PostgreSQL" << Qt::endl;
        return 1;
    }
    out << "Connected to PostgreSQL" << Qt::endl;

    // Apply migrations.
    if (!pip.applyMigrations(QStringLiteral("sql/V001__schema.sql"), QStringLiteral("sql/V002__seed.sql")))
    {
        out << "WARNING: Migration failed (tables may already exist)" << Qt::endl;
    }

    // Build components.
    auth::Authenticator auth(pip, cfg);
    session::SessionManager sessions(pip, cfg);
    pdp::PDP pdp(pip);
    pep::PEP pep(pip, auth, sessions, pdp);

    // Start listener.
    server::Listener listener(cfg, pep);
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
