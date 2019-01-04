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

static QString s_discoveryPrefix = "homeassistant";

Entity::Entity(QMqttClient *client, QObject *parent):
    QObject(parent),
    m_client(client)
{
    connect(m_client, &QMqttClient::connected, this, [this]() {
        sendRegistration();
        setInitialState();
    });
}

QString Entity::hostname() const
{
    return QHostInfo::localHostName().toLower();
}

QString Entity::baseTopic() const
{
    return hostname() + "/" + id();
}

QMqttClient *Entity::client() const
{
    return m_client;
}

void Entity::sendRegistration()
{
    if (haType().isEmpty()) {
        return;
    }
    QVariantMap config = haConfig();
    config["name"] = QHostInfo::localHostName() + " " + name();
    //TODO the new HA device crap

    if (haSetAvailability()) {
        config["availability_topic"] = hostname() + "/connected";
        config["payload_available"] = "on";
        config["payload_not_available"] = "off";
    }

    qDebug() <<  s_discoveryPrefix + "/" + haType() + "/" + hostname() + "/" + id() + "/config";
    m_client->publish(s_discoveryPrefix + "/" + haType() + "/" + hostname() + "/" + id() + "/config", QJsonDocument(QJsonObject::fromVariantMap(config)).toJson(QJsonDocument::Compact), 0, true);
}

ConnectedNode::ConnectedNode(QMqttClient *c, QObject *parent):
    Entity(c, parent)
{
    c->setWillTopic(baseTopic());
    c->setWillMessage("off");
    c->setWillRetain(true);
}

ConnectedNode::~ConnectedNode()
{
    client()->publish(baseTopic(), "off");
}

void ConnectedNode::setInitialState()
{
    client()->publish(baseTopic(), "on");
}

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

