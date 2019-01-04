#include <QApplication>
#include <QDebug>
#include <QMqttClient>
#include <QtGlobal>

#include "components.h"

class HAClient : public QObject {
public:
    HAClient();
    ~HAClient();
private:
    QMqttClient *m_client;

    void updateStatus(const QByteArray &status);
};


HAClient::HAClient() {
    m_client = new QMqttClient(this);
    m_client->setHostname("192.168.1.2");
    m_client->setPort(1883);
    m_client->setUsername("homeassistant");
    m_client->setPassword("wednesday");

    //read from config for enable or not
    new ConnectedNode(m_client, this);
    new ActiveNode(m_client, this);
    new NotificationNode(m_client, this);

    m_client->connectToHost();

}

HAClient::~HAClient()
{
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
 Battery

 */




// $hostname/locked
