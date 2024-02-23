#include "components.h"

#include <QHostInfo>
#include <QMqttClient>
#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <KNotification>
#include <KIdleTime>

#include <QDBusConnection>
#include "login1_manager_interface.h"



Notifications::Notifications(QMqttClient *c, QObject *parent):
    Entity(c, parent)
{
    connect(c, &QMqttClient::connected, this, [this]() {
        auto watcher = client()->subscribe(baseTopic());
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

ActiveSensor::ActiveSensor(QMqttClient *c, QObject *parent):
    Entity(c, parent)
{
    auto kidletime = KIdleTime::instance();
    auto id = kidletime->addIdleTimeout(5 * 60 * 1000);
    connect(kidletime, &KIdleTime::resumingFromIdle, this, [this]() {
        client()->publish(baseTopic(), "active");
    });
    connect(kidletime, &KIdleTime::timeoutReached, this, [this, id, kidletime](int _id) {
        if (_id != id) {
            return;
        }
        client()->publish(baseTopic(), "idle");
        kidletime->catchNextResumeEvent();
    });
}

void ActiveSensor::setInitialState()
{
    client()->publish(baseTopic(), "active");
}

SuspendSwitch::SuspendSwitch(QMqttClient *client, QObject *parent)
    : Entity(client, parent)
{
    connect(client, &QMqttClient::connected, this, [this]() {
        auto watcher = this->client()->subscribe(baseTopic());
        connect(watcher, &QMqttSubscription::messageReceived, this, []() {
            OrgFreedesktopLogin1ManagerInterface logind(QStringLiteral("org.freedesktop.login1"),
                                                        QStringLiteral("/org/freedesktop/login1"),
                                                        QDBusConnection::systemBus());

            logind.Suspend(true).waitForFinished();
        });
    });
}

LockedState::LockedState(QMqttClient *client, QObject *parent)
    : Entity(client, parent)
{
    QDBusConnection::sessionBus().connect(QStringLiteral("org.freedesktop.ScreenSaver"),
                                          QStringLiteral("/ScreenSaver"),
                                          QStringLiteral("org.freedesktop.ScreenSaver"),
                                          QStringLiteral("ActiveChanged"),
                                          this, SLOT(screenLockedChanged(bool)));

    connect(client, &QMqttClient::connected, this, [this]() {
        auto watcher = this->client()->subscribe(baseTopic() + "/set");
        connect(watcher, &QMqttSubscription::messageReceived, this, &LockedState::screenLockStateRequested);
    });

}

void LockedState::setInitialState()
{
    auto isLocked = QDBusMessage::createMethodCall("org.freedesktop.ScreenSaver",
                                   "/ScreenSaver",
                                   "org.freedesktop.ScreenSaver",
                                   "GetActive");
    auto pendingCall = QDBusConnection::sessionBus().asyncCall(isLocked);
    pendingCall.waitForFinished();
    bool locked = pendingCall.reply().arguments().at(0).toBool();
    screenLockedChanged(locked);
}

void LockedState::screenLockedChanged(bool active)
{
    if (active) {
        client()->publish(baseTopic(), "locked");
    } else {
        client()->publish(baseTopic(), "unlocked");
    }
}

void LockedState::screenLockStateRequested(const QMqttMessage &message)
{
    OrgFreedesktopLogin1ManagerInterface login1Session(QStringLiteral("org.freedesktop.login1"),
                                                 QStringLiteral("/org/freedesktop/session/auto"),
                                                 QDBusConnection::systemBus());
    if (message.payload() == "locked") {
        // login1Session.Lock();
    } else if (message.payload() == "unlocked") {
        // login1Session.Unlock();
    }
}
