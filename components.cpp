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
REGISTER_PLUGIN(Notifications);

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
REGISTER_PLUGIN(ActiveSensor);

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
REGISTER_PLUGIN(SuspendSwitch);

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
REGISTER_PLUGIN(LockedState);

Scripts::Scripts(QObject *parent)
    : QObject(parent)
{
    qInfo() << "Loading scripts";
    auto scriptConfigToplevel = KSharedConfig::openConfig()->group("Scripts");
    const QStringList scriptIds = scriptConfigToplevel.groupList();
    for (const QString &scriptId : scriptIds) {
        auto scriptConfig = scriptConfigToplevel.group(scriptId);
        const QString name = scriptConfig.readEntry("Name", scriptId);
        const QString exec = scriptConfig.readEntry("Exec");

        if (exec.isEmpty()) {
            qWarning() << "Could not find script Exec entry for" << scriptId;
            continue;
        }

        auto button = new Button(this);
        button->setId(scriptId);
        button->setName(name);
        // Home assistant integration supports payloads, which we could expose as args
        // maybe via some substitution in the exec line
        connect(button, &Button::triggered, this, [exec, scriptId]() {
            qInfo() << "Running script " << scriptId;
            // DAVE TODO flatpak escaping
            QProcess::startDetached(exec);
        });
    }
}
REGISTER_PLUGIN(Scripts);


Shortcuts::Shortcuts(QObject *parent)
{
    auto shortcutConfigToplevel = KSharedConfig::openConfig()->group("Shortcuts");
    const QStringList shortcutIds = shortcutConfigToplevel.groupList();
    for (const QString &shortcutId : shortcutIds) {
        auto shortcutConfig = shortcutConfigToplevel.group(shortcutId);
        const QString name = shortcutConfig.readEntry("Name", shortcutId);
        QAction *action = new QAction(name, this);
        action->setObjectName(shortcutId);

        auto event = new Event(this);
        event->setId(shortcutId);
        event->setName(name);

        KGlobalAccel::self()->setShortcut(action, {});
        connect(action, &QAction::triggered, event, &Event::trigger);
    }
}

REGISTER_PLUGIN(Shortcuts);
