#include "core.h"
#include "dbusproperty.h"
#include <QCoreApplication>

void setupDndSensor()
{
    auto sensor = new BinarySensor(qApp);
    sensor->setId("dnd");
    sensor->setName("Do not disturb");

    auto dnd = new DBusProperty("org.freedesktop.Notifications", "/org/freedesktop/Notifications", "org.freedesktop.Notifications", "Inhibited", qApp);
    QObject::connect(dnd, &DBusProperty::valueChanged, qApp, [sensor](const QVariant &value) {
        sensor->setState(value.toBool());
    });
    sensor->setState(dnd->value().toBool());
}

REGISTER_INTEGRATION(setupDndSensor)
