#include <QCoreApplication>
#include <QTextStream>

#include <netaccess/common.h>

int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);

    QTextStream out(stdout);
    out << "Hello from netaccess_server " << netaccess::versionString() << Qt::endl;

    return 0;
}
