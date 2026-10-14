/*
    SPDX-License-Identifier: GPL-2.0-or-later
    SPDX-FileCopyrightText: 2026 Miryu Dynamic Lock contributors
*/
#ifndef MIRYU_BLUEZ_H
#define MIRYU_BLUEZ_H

#include <QObject>
#include <QString>
#include <QHash>
#include <QList>
#include <QDBusObjectPath>
#include <QDBusMessage>

/**
 * @brief Bridge to the BlueZ D-Bus daemon.
 *
 * Enumerates paired Bluetooth devices and emits a signal whenever any of the
 * properties that describe their proximity changes (the connection state or the
 * measured signal strength), so the daemon can react when the paired phone is
 * taken away.
 */
class MiryuBluez : public QObject
{
    Q_OBJECT
public:
    struct BluezDevice {
        QString path;
        QString address; ///< hardware address, e.g. "AA:BB:CC:DD:EE:FF"
        QString name; ///< device friendly name
        QString alias; ///< adapter-set alias
        bool connected = false;
        bool paired = false;
        int rssi = 0; ///< last measured signal strength in dBm (0 when unknown)
        bool rssiValid = false; ///< true when BlueZ reported a usable RSSI
    };

    typedef QHash<QString, QVariantMap> InterfaceMap; ///< interface name -> properties
    typedef QHash<QDBusObjectPath, InterfaceMap> ObjectMap; ///< object path -> interfaces

    explicit MiryuBluez(QObject *parent = nullptr);

    /// List all paired Bluetooth devices known to BlueZ.
    QList<BluezDevice> listPairedDevices();
    /// Find the paired device with @p address. @return true when it was found,
    /// in which case @p device is filled in.
    bool findDevice(const QString &address, BluezDevice &device);
    /// Resolve the device object path for @p address, empty if unknown.
    QString pathForAddress(const QString &address);

Q_SIGNALS:
    /// Emitted when the connection state or the signal strength of a BlueZ
    /// device changes.
    void deviceStateChanged(const QString &address);

private Q_SLOTS:
    void onPropertiesChanged(const QDBusMessage &message);

private:
    void setupSubscription();
    QString addressForPath(const QString &path) const;
    QHash<QString, QString> m_addressByPath; ///< object path -> hardware address
};

Q_DECLARE_METATYPE(MiryuBluez::BluezDevice)

#endif // MIRYU_BLUEZ_H
