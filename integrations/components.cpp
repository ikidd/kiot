#include "components.h"

#include <QHostInfo>
#include <QMqttClient>
#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <KNotification>
#include <KIdleTime>

#include <KSharedConfig>
#include <KConfigGroup>

#include <QAction>
#include <KGlobalAccel>

#include <QDBusConnection>
#include "login1_manager_interface.h"

Notifications::Notifications(QObject *parent):
    Entity(parent)
{
    setId("notifications");
    setName("Notifications");

    connect(HaControl::mqttClient(), &QMqttClient::connected, this, [this]() {
        auto watcher = HaControl::mqttClient()->subscribe(baseTopic());
        connect(watcher, &QMqttSubscription::messageReceived, this, &Notifications::notificationCallback);
    });
}

void Notifications::notificationCallback(const QMqttMessage &message)
{
    auto docs = QJsonDocument::fromJson(message.payload());
    auto objs = docs.object();
    const QString title = objs["title"].toString();
    const QString body = objs["message"].toString();
    KNotification::event(KNotification::Notification, title, body);
}

ActiveSensor::ActiveSensor(QObject *parent):
    QObject(parent)
{
    m_sensor.setId("active");
    m_sensor.setName("Active");
    m_sensor.setDiscoveryConfig("device_class", "presence");

    // mark as idle after 1 minute. Then in HA to detect a 5 minute idle-ness, wait till we're in this state for 4 minutes

    auto kidletime = KIdleTime::instance();
    auto id = kidletime->addIdleTimeout(60 * 1000);
    connect(kidletime, &KIdleTime::resumingFromIdle, this, [this]() {
        m_sensor.setState(true);
    });
    connect(kidletime, &KIdleTime::timeoutReached, this, [this, id, kidletime](int _id) {
        if (_id != id) {
            return;
        }
        m_sensor.setState(false);
        kidletime->catchNextResumeEvent();
    });
    m_sensor.setState(true);
}

SuspendSwitch::SuspendSwitch(QObject *parent)
    : QObject(parent)
{
    m_button.setId("suspend");
    m_button.setName("Suspend");

    connect(&m_button, &Button::triggered, this, []() {
        OrgFreedesktopLogin1ManagerInterface logind(QStringLiteral("org.freedesktop.login1"),
                                                    QStringLiteral("/org/freedesktop/login1"),
                                                    QDBusConnection::systemBus());
        logind.Suspend(true).waitForFinished();
    });
}

LockedState::LockedState(QObject *parent)
    : QObject(parent)
{
    m_locked.setId("locked");
    m_locked.setName("Locked");
    m_locked.setDiscoveryConfig("device_class", "lock");

    // why am I used freedesktop here, and logind later.... I don't know
    QDBusConnection::sessionBus().connect(QStringLiteral("org.freedesktop.ScreenSaver"),
                                          QStringLiteral("/ScreenSaver"),
                                          QStringLiteral("org.freedesktop.ScreenSaver"),
                                          QStringLiteral("ActiveChanged"),
                                          this, SLOT(screenLockedChanged(bool)));

    connect(&m_locked, &Switch::stateChangeRequested, this, &LockedState::stateChangeRequested);

    auto isLocked = QDBusMessage::createMethodCall("org.freedesktop.ScreenSaver",
                                   "/ScreenSaver",
                                   "org.freedesktop.ScreenSaver",
                                   "GetActive");
    auto pendingCall = QDBusConnection::sessionBus().asyncCall(isLocked);
    pendingCall.waitForFinished();
    const bool locked = pendingCall.reply().arguments().at(0).toBool();
    m_locked.setState(locked);
}

void LockedState::screenLockedChanged(bool active)
{
    m_locked.setState(active);
}

void LockedState::stateChangeRequested(bool state)
{
    if (state) {
        QDBusMessage lock = QDBusMessage::createMethodCall("org.freedesktop.login1",
                                                           "/org/freedesktop/login1/session/auto",
                                                           "org.freedesktop.login1.Session",
                                                           "Lock");
        QDBusConnection::systemBus().asyncCall(lock);
    } else {
        QDBusMessage unlock = QDBusMessage::createMethodCall("org.freedesktop.login1",
                                                             "/org/freedesktop/login1/session/auto",
                                                             "org.freedesktop.login1.Session",
                                                             "Unlock");
        QDBusConnection::systemBus().asyncCall(unlock);
    }
}

void registerLockedState()
{
    new LockedState(qApp);
}

REGISTER_INTEGRATION(registerLockedState)


