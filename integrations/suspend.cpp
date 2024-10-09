#include "core.h"
#include <QCoreApplication>
#include <QDBusConnection>
#include "login1_manager_interface.h"

void setupSuspend()
{
    Button *button = new Button(qApp);
    button->setId("suspend");
    button->setName("Suspend");

    QObject::connect(button, &Button::triggered, qApp, []() {
        OrgFreedesktopLogin1ManagerInterface logind(QStringLiteral("org.freedesktop.login1"),
                                                    QStringLiteral("/org/freedesktop/login1"),
                                                    QDBusConnection::systemBus());
        logind.Suspend(true).waitForFinished();
    });
}

REGISTER_INTEGRATION(setupSuspend)
