#pragma once

#include <QString>
#include <QObject>
#include <QVariantMap>
#include <QMqttClient>

class Node: public QObject
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
    Node(QMqttClient *client, QObject *parent);
    QString hostname() const;
    QString buildTopic() const;
    QMqttClient *client() const;
private:
    QMqttClient *m_client;
};


class ConnectedNode: public Node
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
            {"state_topic", buildTopic()},
            {"payload_on", "on"},
            {"payload_off", "off"},
            {"device_class", "power"}
        });
    }
    bool haSetAvailability() const override {
        return false;
    }
    void setInitialState() override;
};

class ActiveNode: public Node
{
    Q_OBJECT
public:
    ActiveNode(QMqttClient *client, QObject *parent);
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
            {"state_topic", buildTopic()},
            {"payload_on", "active"},
            {"payload_off", "idle"},
            {"device_class", "presence"}
        };
    }
    void setInitialState() override;
};

class NotificationNode: public Node
{
    Q_OBJECT
public:
    NotificationNode(QMqttClient *client, QObject *parent);
    QString id() const override {
        return QStringLiteral("notifications");
    }
    QString name () const override {
        return QStringLiteral("Notifications");
    }
    QString haType() const override {
        return "notifications"; //HA notifications discovery doesn't exist (YET), so this doesn't really work
    }
    void notificationCallback(const QMqttMessage &message);
};

