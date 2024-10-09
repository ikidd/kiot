#pragma once

#include <QString>
#include <QObject>
#include <QVariantMap>
#include <QMqttClient>

#include "core.h"

class ActiveSensor: public QObject
{
    Q_OBJECT
public:
    Q_INVOKABLE ActiveSensor(QObject *parent);
private:
    BinarySensor m_sensor;
};

class Notifications: public Entity
{
    Q_OBJECT
public:
    Q_INVOKABLE Notifications(QObject *parent);

    void notificationCallback(const QMqttMessage &message);
};

class SuspendSwitch : public QObject
{
    Q_OBJECT
public:
    Q_INVOKABLE SuspendSwitch(QObject *parent);
private:
    Button m_button;
};

class LockedState : public QObject
{
    Q_OBJECT
public:
    Q_INVOKABLE LockedState(QObject *parent);

private Q_SLOTS:
    void screenLockedChanged(bool active);
    void stateChangeRequested(bool state);
private:
    Switch m_locked;
};
