#pragma once

#include <QString>
#include <QObject>
#include <QVariantMap>
#include <QMqttClient>

#include "core.h"

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
