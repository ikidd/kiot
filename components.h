#pragma once

#include <QString>
#include <QObject>
#include <QVariantMap>
#include <QMqttClient>

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

//temp
// class WriteNode {
// public:
//     WriteNode(Entity *entity, const QString &topic);
//     QString path();
//     void write(const QString &payload);
// };
// class WatchNode : public QObject {
// Q_OBJECT
// public:
//     WatchNode(Entity *entity, const QString &topic);
// Q_SIGNALS:
//     void changed(const QString &payload);
// private:
//
// };


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

class ActiveSensor: public Entity
{
    Q_OBJECT
public:
    ActiveSensor(QMqttClient *client, QObject *parent);
    QString id() const override {
        return QStringLiteral("active");
    }
    QString name () const override {
        return QStringLiteral("Active");
    }
    QString haType() const override {
        return QStringLiteral("binary_sensor");
    };
    QVariantMap haConfig() const override {
        return {
            {"state_topic", baseTopic()},
            {"payload_on", "active"},
            {"payload_off", "idle"},
            {"device_class", "presence"}
        };
    }
    void setInitialState() override;
};

class Notifications: public Entity
{
    Q_OBJECT
public:
    Notifications(QMqttClient *client, QObject *parent);
    QString id() const override {
        return QStringLiteral("notifications");
    }
    QString name () const override {
        return QStringLiteral("Notifications");
    }
    QString haType() const override {
        return ""; //HA notifications discovery doesn't exist (YET), so this doesn't really work
    }
    void notificationCallback(const QMqttMessage &message);
};

class SuspendSwitch : public Entity
{
    Q_OBJECT
public:
    SuspendSwitch(QMqttClient *client, QObject *parent);
    QString id() const override {
        return QStringLiteral("suspend");
    }
    QString name () const override {
        return QStringLiteral("Suspend");
    }
    QString haType() const override {
        return QStringLiteral("button");
    }
    QVariantMap haConfig() const override {
        return {
            {"command_topic", baseTopic()}
        };
    }
};

class LockedState : public Entity
{
    Q_OBJECT
public:
    LockedState(QMqttClient *client, QObject *parent);
    QString id() const override {
        return QStringLiteral("locked");
    }
    QString name () const override {
        return QStringLiteral("Locked");
    }
    QString haType() const override {
        return QStringLiteral("switch");
    }
    QVariantMap haConfig() const override {
        return {
            {"state_topic", baseTopic()},
            {"command_topic", baseTopic() + "/set"},
            {"payload_on", "locked"},
            {"payload_off", "unlocked"},
            {"device_class", "lock"}
        };
    }
    void setInitialState() override;
private Q_SLOTS:
    void screenLockedChanged(bool active);
    void screenLockStateRequested(const QMqttMessage &message);
};
