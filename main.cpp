#include <QApplication>
#include <QDebug>

#include <KAboutData>
#include <KSharedConfig>
#include <KConfigGroup>
#include <KDBusService>

#include "core.h"

int main(int argc, char ** argv)
{
    QApplication app(argc, argv);
    KAboutData aboutData(
        QStringLiteral("kde-ha"),
        "KDE HA",
        QStringLiteral("0.1"),
        "KDE Home Automation Connect",
        KAboutLicense::GPL_V3,
        "© 2024",
        "HA Deamon"
    );
    KDBusService service(KDBusService::Unique);
    HaControl appControl;
    app.exec();
}
