#include "dbusproperty.h"

#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusPendingCall>
#include <QDBusPendingReply>

DBusProperty::DBusProperty(const QString &service, const QString &path, const QString &interface, const QString &property, QObject *parent)
    : QObject(parent)
    , m_service(service)
    , m_path(path)
    , m_interface(interface)
    , m_property(property)
{
    QDBusConnection::sessionBus().connect(m_service, m_path, "org.freedesktop.DBus.Properties", "PropertiesChanged", this, SLOT(onFdoPropertiesChanged(QString,QVariantMap,QStringList)));

    auto message = QDBusMessage::createMethodCall(m_service, m_path, "org.freedesktop.DBus.Properties", "Get");
    message << m_interface << m_property;

    auto reply = QDBusConnection::sessionBus().call(message);
    if (reply.type() == QDBusMessage::ReplyMessage) {
        m_value = reply.arguments().at(0);
    }
}

QVariant DBusProperty::value() const
{
    return m_value;
}

void DBusProperty::onFdoPropertiesChanged(const QString &interface, const QVariantMap &changed, const QStringList &invalidated)
{
    Q_UNUSED(interface)
    Q_UNUSED(invalidated)

    if (changed.contains(m_property)) {
        m_value = changed[m_property];
        Q_EMIT valueChanged(m_value);
    }
}
