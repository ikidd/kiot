#include "core.h"

#include <QDebug>
#include <QMqttClient>
#include <QtGlobal>
#include <QHostInfo>

#include <KConfig>
#include <KSharedConfig>
#include <KConfigGroup>

#include "core.h"
#include "components.h"


HaControl::HaControl() {
    s_self = this;
    auto config = KSharedConfig::openConfig();
    qDebug() << config->name();
    auto group = config->group("general");
    m_client = new QMqttClient(this);
    m_client->setHostname(group.readEntry("host"));
    m_client->setPort(group.readEntry("port", 1883));
    m_client->setUsername(group.readEntry("user"));
    m_client->setUsername(group.readEntry("password"));

           //read from config for enable or not
    new ConnectedNode(m_client, this);
    new ActiveSensor(m_client, this);
    new Notifications(m_client, this);
    new SuspendSwitch(m_client, this);
    new LockedState(m_client, this);

    auto connectToHost = [this]() {
        // if(m_networkConfigurationManager.isOnline()) {
        m_client->connectToHost();
        // }
    };


           //
           // connect(&m_networkConfigurationManager, &QNetworkConfigurationManager::configurationChanged, this, connectToHost);
           //
    connect(m_client, &QMqttClient::stateChanged, this, [](QMqttClient::ClientState state) {
        switch (state) {
        case QMqttClient::Connected:
            qDebug() << "connected";
            break;
        case QMqttClient::Connecting:
            qDebug() << "connecting";
            break;
        case QMqttClient::Disconnected:
            qDebug() << "disconnected";
            //do I need to reconnect?
            break;
        }
    });

    connectToHost();
}

HaControl::~HaControl()
{
}

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
    config["name"] = name();
    //TODO the new HA device crap

    if (haSetAvailability()) {
        config["availability_topic"] = hostname() + "/connected";
        config["payload_available"] = "on";
        config["payload_not_available"] = "off";
    }
    config["device"] = QVariantMap({{"identifiers", "linux_ha_bridge_" + hostname() }});
    config["unique_id"] = "linux_ha_control_"+ hostname() + "_" + id();

    qDebug() <<  s_discoveryPrefix + "/" + haType() + "/" + hostname() + "/" + id() + "/config" << config;
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
