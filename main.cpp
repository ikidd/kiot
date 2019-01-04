#include <QApplication>
#include <QDebug>
#include <QMqttClient>
#include <QtGlobal>
// #include <QNetworkConfigurationManager>

#include <KConfig>
#include <KSharedConfig>
#include <KConfigGroup>

#include "components.h"

class HAClient : public QObject {
public:
    HAClient();
    ~HAClient();
private:
    QMqttClient *m_client;
    // QNetworkConfigurationManager m_networkConfigurationManager;

    void updateStatus(const QByteArray &status);
};

HAClient::HAClient() {
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

HAClient::~HAClient()
{
}

int main(int argc, char ** argv)
{
    //TODO app name and stuff
    QApplication app(argc, argv);
    HAClient client;
    app.exec();
}


/**
 * Topics
 *
 * READ:
 * $hostname/connected [on/off]
 * $hostname/active [active/idle] (set after 5 minutes)

 * Note: you can use the update time of active to calculate how long we've been inactive, remember to add 5minutes

 * WRITE:
 * $hostname/suspend/set [on]
 * $hostname/notifications JSON{"title":"someTitle", "message": "someMessage"}

 Konversation queue (knotification plugin....or manually add Execute line in the relevant place?)
 Deliberately Invasive UI (i.e prevent work until you empty the washing machine - like rsibreak)
 Shortcut pressed sensor?
 Lock status?
 Battery?
 Only try to connect on certain wifi networks?

 Config
 [general]
 host=some.host
 port=1883
 user=myUsername
 password=myPassword

 */




// $hostname/locked
