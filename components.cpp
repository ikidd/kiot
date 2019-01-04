#include "components.h"

#include <QHostInfo>
#include <QMqttClient>
#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <KNotification>
#include <KIdleTime>

static QString s_discoveryPrefix = "homeassistant";

Node::Node(QMqttClient *client, QObject *parent):
    QObject(parent),
    m_client(client)
{
    connect(m_client, &QMqttClient::connected, this, [this]() {
        sendRegistration();
        setInitialState();
    });
}

QString Node::hostname() const
{
    return QHostInfo::localHostName().toLower();
}

QString Node::buildTopic() const
{
    return hostname() + "/" + id();
}

QMqttClient *Node::client() const
{
    return m_client;
}

void Node::sendRegistration()
{
    QVariantMap config = haConfig();
    config["name"] = QHostInfo::localHostName() + " " + name();
    //TODO the new HA device crap

    if (haSetAvailability()) {
        config["availability_topic"] = hostname() + "/connected";
        config["payload_available"] = "on";
        config["payload_not_available"] = "off";
    }

    m_client->publish(s_discoveryPrefix + "/" + haType() + "/" + hostname() + "/" + id() + "/config", QJsonDocument(QJsonObject::fromVariantMap(config)).toJson(QJsonDocument::Compact), 0, true);
}

ConnectedNode::ConnectedNode(QMqttClient *c, QObject *parent):
    Node(c, parent)
{
    c->setWillTopic(buildTopic());
    c->setWillMessage("off");
    c->setWillRetain(true);
}

ConnectedNode::~ConnectedNode()
{
    client()->publish(buildTopic(), "off");
}

void ConnectedNode::setInitialState()
{
    client()->publish(buildTopic(), "on");
}

NotificationNode::NotificationNode(QMqttClient *c, QObject *parent):
    Node(c, parent)
{
    c->connect(c, &QMqttClient::connected, this, [this]() {
        auto watcher = client()->subscribe(buildTopic());
        connect(watcher, &QMqttSubscription::messageReceived, this, &NotificationNode::notificationCallback);
    });
}

void NotificationNode::notificationCallback(const QMqttMessage &message)
{
    auto docs = QJsonDocument::fromJson(message.payload());
    auto objs = docs.object();
    const QString title = objs["title"].toString();
    const QString body = objs["message"].toString();
    KNotification::event(KNotification::Notification, title, body);
}

ActiveNode::ActiveNode(QMqttClient *c, QObject *parent):
    Node(c, parent)
{
    auto kidletime = KIdleTime::instance();
    auto id = kidletime->addIdleTimeout(5 * 60 * 1000);
    connect(kidletime, &KIdleTime::resumingFromIdle, this, [this]() {
        client()->publish(buildTopic(), "active");
    });
    connect(kidletime, QOverload<int>::of(&KIdleTime::timeoutReached), this, [this, id](int _id) {
        if (_id != id) {
            return;
        }
        client()->publish(buildTopic(), "idle");
    });
}

void ActiveNode::setInitialState()
{
    client()->publish(buildTopic(), "active");
}
