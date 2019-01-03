#include <QApplication>
#include <QDebug>
#include <QMqttClient>
#include <QDBusConnection>
#include <QtGlobal>

#include <QJsonDocument>
#include <QJsonObject>
#include <QHostInfo>

#include <KIdleTime>
#include <KNotification>

class HAClient : public QObject {
public:
    HAClient();
    ~HAClient();
private:
    QMqttClient *m_client;

    QString buildTopicBase(const QString &component, const QString &objectId) const;

    void notificationCallback(const QMqttMessage &message);
    void setStatusCallback(const QMqttMessage &message);
    void updateStatus(const QByteArray &status);
    QString m_hostname;

    QString m_discoveryPrefix = "homeassistant";
};


HAClient::HAClient() {
    m_hostname = QHostInfo::localHostName();

    m_client = new QMqttClient(this);
    m_client->setHostname("192.168.1.2");
    m_client->setPort(1883);
    m_client->setUsername("homeassistant");
    m_client->setPassword("wednesday");
    m_client->connectToHost();

    m_client->setWillTopic(m_hostname + "/connected");
    m_client->setWillMessage("off");
    m_client->setWillRetain(true);

    m_client->publish(QMqttTopicName(m_hostname + "/connected"), "on");

    m_client->connect(m_client, &QMqttClient::connected, [this]() {
        {
            auto watcher = m_client->subscribe(QMqttTopicFilter(m_hostname + "/notification"));
            connect(watcher, &QMqttSubscription::messageReceived, this, &HAClient::notificationCallback);
        }

        m_client->publish(QMqttTopicName(m_hostname + "/connected"), "on");

        //register to HA
        QJsonObject config;
        config["name"] = "Connected";
        config["state_topic"] = m_hostname + "/connected";
        config["payload_on"] = "on";
        config["payload_off"] = "off";
        config["device_class"] = "power";
        m_client->publish(m_discoveryPrefix + "/binary_sensor/" + m_hostname + "/connected/config", QJsonDocument(config).toJson(QJsonDocument::Compact), 0, true);

        QJsonObject isActiveConfig;
        isActiveConfig["name"] = "Active";
        isActiveConfig["state_topic"] = m_hostname + "/status";
        isActiveConfig["payload_off"] = "idle";
        isActiveConfig["payload_on"] = "active";
        isActiveConfig["availability_topic"] = m_hostname + "/connected";
        isActiveConfig["payload_available"] = "on";
        isActiveConfig["payload_not_available"] = "off";
        isActiveConfig["device_class"] = "presence";
        m_client->publish(m_discoveryPrefix + "/binary_sensor/" + m_hostname + "/active/config", QJsonDocument(isActiveConfig).toJson(QJsonDocument::Compact), 0, true);

        QJsonObject switchConfig;
        switchConfig["name"] = "Suspend";
        switchConfig["command_topic"] = m_hostname + "/status/set";
        switchConfig["payload_on"] = "suspend";
        switchConfig["availability_topic"] = m_hostname + "/connected";
        switchConfig["payload_available"] = "on";
        switchConfig["payload_not_available"] = "off";
        m_client->publish(m_discoveryPrefix + "/switch/" + m_hostname + "/suspend/config", QJsonDocument(switchConfig).toJson(QJsonDocument::Compact), 0, true);

        updateStatus("active");
    });

    auto kidletime = KIdleTime::instance();
    auto id = kidletime->addIdleTimeout(5 * 60 * 1000);
    connect(kidletime, &KIdleTime::resumingFromIdle, this, [this]() {
        updateStatus("active");
    });
    connect(kidletime, QOverload<int>::of(&KIdleTime::timeoutReached), this, [this, id](int _id) {
        if (_id != id) {
            return;
        }
        updateStatus("idle");
    });
}

HAClient::~HAClient()
{
    m_client->publish(QMqttTopicName(m_hostname + "/connected"), "off");
}

void HAClient::notificationCallback(const QMqttMessage &message)
{
    auto docs = QJsonDocument::fromJson(message.payload());
    auto objs = docs.object();
    const QString title = objs["title"].toString();
    const QString body = objs["message"].toString();
    KNotification::event(KNotification::Notification, title, body);
}

void HAClient::setStatusCallback(const QMqttMessage &message)
{
    const QByteArray payload = message.payload();
    if (payload == "suspend") {
    }
}

void HAClient::updateStatus(const QByteArray &status)
{
    m_client->publish(QMqttTopicName(m_hostname + "/status"), status);
}

int main(int argc, char ** argv)
{
    QApplication app(argc, argv);
    HAClient client;
    app.exec();
}


/**
 * Commands
 *
 * GET:
 * $hostname/connected [online/offfline]
 * $hostname/idle [active/idle]

 * SET
 * $hostname/suspend/set [on]
 * $hostname/notification JSON{"title":"someTitle", "message": "someMessage"}

 Exposed to HA as:
 [switch connected]

 TODO:
 Locked status
 Batter

 */




// $hostname/locked
