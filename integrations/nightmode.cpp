#include "core.h"
#include "dbusproperty.h"
#include <QCoreApplication>

void setupNightmode()
{
    auto sensor = new BinarySensor(qApp);
    sensor->setId("nightmode_inhibited");
    sensor->setName("Night mode inhibited");

    auto nightmodeInhibited = new DBusProperty("org.kde.KWin", "/org/kde/KWin/NightLight", "org.kde.KWin.NightLight", "inhibited", qApp);
    QObject::connect(nightmodeInhibited, &DBusProperty::valueChanged, qApp, [sensor](const QVariant &value) {
        sensor->setState(value.toBool());
    });
    sensor->setState(nightmodeInhibited->value().toBool());

    // current temp?
    // enabled?
    // create inhibitions?
}

REGISTER_INTEGRATION(setupNightmode)
