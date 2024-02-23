#include <QObject>
#include <QVariantMap>

class QMqttClient;

class HaControl : public QObject {
public:
    ~HaControl();
    static QMqttClient *mqttClient() {
        return s_self->m_client;
    }
private:
    HaControl();
    static HaControl *s_self;
    QMqttClient *m_client;
    // QNetworkConfigurationManager m_networkConfigurationManager;
};

class Entity: public QObject
{
    Q_OBJECT
public:
    virtual QString id() const = 0;
    virtual QString name() const = 0;
    virtual void setInitialState() {}

    virtual QString haType() const = 0;
    virtual QVariantMap haConfig() const {
        return {};
    }
    virtual bool haSetAvailability() const {
        return true;
    }
    void sendRegistration();
protected:
    Entity(QMqttClient *client, QObject *parent);
    QString hostname() const;
    QString baseTopic() const;
    QMqttClient *client() const;
private:
    QMqttClient *m_client;
};


class ConnectedNode: public Entity
{
    Q_OBJECT
public:
    ConnectedNode(QMqttClient *client, QObject *parent);
    ~ConnectedNode();
    QString id() const override {
        return QStringLiteral("connected");
    }
    QString name () const override {
        return QStringLiteral("Connected");
    }
    QString haType() const override {
        return QStringLiteral("binary_sensor");
    };
    QVariantMap haConfig() const override {
        return QVariantMap({
            {"state_topic", baseTopic()},
            {"payload_on", "on"},
            {"payload_off", "off"},
            {"device_class", "power"},
            {"device", QVariantMap({
                           {"name", hostname() },
                           {"identifiers", "linux_ha_bridge_" + hostname() },
                           {"sw_version", "0.1"},
                           {"manufacturer", "Linux HA Bridge"},
                           {"model", "Linux"}
                       })}
        });
    }
    bool haSetAvailability() const override {
        return false;
    }
    void setInitialState() override;
};
