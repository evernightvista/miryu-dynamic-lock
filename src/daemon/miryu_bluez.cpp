/*
    SPDX-License-Identifier: GPL-2.0-or-later
    SPDX-FileCopyrightText: 2026 Miryu Dynamic Lock contributors
*/
#include "miryu_bluez.h"
#include "miryu_config_keys.h"

#include <QDBusArgument>
#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusReply>
#include <QDBusVariant>
#include <QDBusMetaType>
#include <QLoggingCategory>

Q_LOGGING_CATEGORY(MIRYU_BLUEZ, "miryu.bluez")

MiryuBluez::MiryuBluez(QObject *parent)
    : QObject(parent)
{
    qDBusRegisterMetaType<InterfaceMap>();
    qDBusRegisterMetaType<ObjectMap>();
    // NOTE: BluezDevice is NOT registered with qDBusRegisterMetaType. It is a
    // local-only struct populated by parsing the ObjectMap reply; it never
    // crosses the D-Bus wire, so it does not need (and must not have) the
    // QDBusArgument << / >> streaming operators that qDBusRegisterMetaType
    // would require. Q_DECLARE_METATYPE in the header is enough for QVariant.
    setupSubscription();
}

void MiryuBluez::setupSubscription()
{
    QDBusConnection bus = QDBusConnection::systemBus();
    if (!bus.isConnected()) {
        qCWarning(MIRYU_BLUEZ) << "System D-Bus is not available; Bluetooth monitoring disabled.";
        return;
    }

    // Receive PropertiesChanged from any object and filter by interface in the
    // handler. The sender service and object path are left empty so we catch
    // every BlueZ device without enumerating paths up front.
    const bool ok = bus.connect(
        QString(), // any service
        QString(), // any path
        QStringLiteral("org.freedesktop.DBus.Properties"),
        QStringLiteral("PropertiesChanged"),
        QStringLiteral("sa{sv}as"),
        this, SLOT(onPropertiesChanged(QDBusMessage)));
    if (!ok) {
        qCWarning(MIRYU_BLUEZ) << "Could not subscribe to BlueZ PropertiesChanged.";
    }
}

QList<MiryuBluez::BluezDevice> MiryuBluez::listPairedDevices()
{
    QList<BluezDevice> devices;
    QDBusConnection bus = QDBusConnection::systemBus();
    if (!bus.isConnected()) {
        return devices;
    }

    QDBusInterface objManager(QString::fromLatin1(Miryu::kBluezService),
                              QString::fromLatin1(Miryu::kBluezRootPath),
                              QString::fromLatin1(Miryu::kObjManagerIface),
                              bus);
    const QDBusReply<ObjectMap> reply = objManager.call(QStringLiteral("GetManagedObjects"));
    if (!reply.isValid()) {
        qCWarning(MIRYU_BLUEZ) << "GetManagedObjects failed:" << reply.error().message();
        return devices;
    }

    const ObjectMap objects = reply.value();
    for (auto it = objects.constBegin(); it != objects.constEnd(); ++it) {
        const InterfaceMap &ifaces = it.value();
        if (!ifaces.contains(QString::fromLatin1(Miryu::kDeviceIface))) {
            continue;
        }
        const QVariantMap props = ifaces.value(QString::fromLatin1(Miryu::kDeviceIface));
        if (!props.value(QStringLiteral("Paired")).toBool()) {
            continue;
        }
        BluezDevice d;
        d.path = it.key().path();
        d.address = props.value(QStringLiteral("Address")).toString();
        d.name = props.value(QStringLiteral("Name")).toString();
        d.alias = props.value(QStringLiteral("Alias")).toString();
        d.connected = props.value(QStringLiteral("Connected")).toBool();
        d.paired = true;
        // RSSI is only present (and non-zero) once BlueZ actually has a reading
        // for the device; treat its absence as "unknown" instead of 0 dBm.
        if (props.contains(QStringLiteral("RSSI"))) {
            d.rssi = props.value(QStringLiteral("RSSI")).toInt();
            d.rssiValid = (d.rssi != 0);
        }
        if (!d.address.isEmpty()) {
            m_addressByPath.insert(d.path, d.address);
        }
        devices.append(d);
    }
    return devices;
}

bool MiryuBluez::findDevice(const QString &address, BluezDevice &device)
{
    const QList<BluezDevice> devices = listPairedDevices();
    for (const BluezDevice &d : devices) {
        if (d.address.toUpper() == address.toUpper()) {
            device = d;
            return true;
        }
    }
    return false;
}

QString MiryuBluez::pathForAddress(const QString &address)
{
    const QList<BluezDevice> devices = listPairedDevices();
    for (const BluezDevice &d : devices) {
        if (d.address.toUpper() == address.toUpper()) {
            return d.path;
        }
    }
    return QString();
}

QString MiryuBluez::addressForPath(const QString &path) const
{
    QDBusInterface props(QString::fromLatin1(Miryu::kBluezService),
                         path,
                         QString::fromLatin1(Miryu::kPropsIface),
                         QDBusConnection::systemBus());
    const QDBusReply<QDBusVariant> reply = props.call(QStringLiteral("Get"),
                                                      QString::fromLatin1(Miryu::kDeviceIface),
                                                      QStringLiteral("Address"));
    if (!reply.isValid()) {
        return QString();
    }
    return reply.value().variant().toString();
}

void MiryuBluez::onPropertiesChanged(const QDBusMessage &message)
{
    const QString path = message.path();
    const QList<QVariant> args = message.arguments();
    if (args.size() < 2) {
        return;
    }
    const QString interface = args.at(0).toString();
    if (interface != QString::fromLatin1(Miryu::kDeviceIface)) {
        return;
    }

    QVariantMap changed;
    const QVariant &second = args.at(1);
    if (second.canConvert<QVariantMap>()) {
        changed = second.value<QVariantMap>();
    } else {
        const QDBusArgument arg = second.value<QDBusArgument>();
        arg >> changed;
    }
    // Both the connection state and the measured signal strength describe how
    // close the phone is, so react to either of them.
    if (!changed.contains(QStringLiteral("Connected")) && !changed.contains(QStringLiteral("RSSI"))) {
        return;
    }

    QString address = m_addressByPath.value(path);
    if (address.isEmpty()) {
        address = addressForPath(path);
        if (!address.isEmpty()) {
            m_addressByPath.insert(path, address);
        }
    }
    if (address.isEmpty()) {
        return;
    }

    qCDebug(MIRYU_BLUEZ) << "Device" << address << "proximity properties changed:" << changed.keys();
    Q_EMIT deviceStateChanged(address);
}
