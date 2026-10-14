/*
    SPDX-License-Identifier: GPL-2.0-or-later
    SPDX-FileCopyrightText: 2026 Miryu Dynamic Lock contributors
*/
#ifndef KCM_MIRYUDYNAMICLOCK_H
#define KCM_MIRYUDYNAMICLOCK_H

#include <KQuickConfigModule>

#include <QDBusObjectPath>
#include <QHash>
#include <QList>
#include <QString>
#include <QVariantList>
#include <QVariantMap>

/**
 * @brief KDE Plasma 6 control module for Miryu Dynamic Lock.
 *
 * Implemented as a QtQuick KCM (KQuickConfigModule) rather than a QWidget
 * KCModule. This is what makes it possible to put the master switch in the page
 * header, to the right of the title: the QML root element exposes the module's
 * properties, and the switch is a checkable Kirigami.Action whose
 * displayComponent is a QQC2.Switch (the same technique plasma-firewall uses in
 * its kcm/ui/main.qml). A QWidget KCModule cannot do this — System Settings only
 * renders a plain text title for those.
 *
 * The QML side reads and writes the properties below through the "kcm" context
 * property that KQuickConfigModule injects.
 *
 * When the host has no Bluetooth support (no BlueZ D-Bus service on the system
 * bus and/or no Bluetooth adapter exposed by BlueZ), the header shows a warning
 * banner, but only while the master switch is on: the user is warned exactly
 * when they try to enable a feature the machine cannot support, and never while
 * the feature is simply left off.
 */
class KCM_MiryuDynamicLock : public KQuickConfigModule
{
    Q_OBJECT

    Q_PROPERTY(bool enabled READ enabled WRITE setEnabled NOTIFY enabledChanged)
    Q_PROPERTY(int lockTimeout READ lockTimeout WRITE setLockTimeout NOTIFY lockTimeoutChanged)
    /// Signal strength (dBm) below which the phone counts as taken away.
    Q_PROPERTY(int rssiThreshold READ rssiThreshold WRITE setRssiThreshold NOTIFY rssiThresholdChanged)
    Q_PROPERTY(bool cancelOnReconnect READ cancelOnReconnect WRITE setCancelOnReconnect NOTIFY cancelOnReconnectChanged)
    Q_PROPERTY(bool notifyBeforeLock READ notifyBeforeLock WRITE setNotifyBeforeLock NOTIFY notifyBeforeLockChanged)
    Q_PROPERTY(QString deviceAddress READ deviceAddress WRITE setDeviceAddress NOTIFY deviceAddressChanged)
    /// true when BlueZ is reachable and at least one adapter is present.
    Q_PROPERTY(bool bluetoothAvailable READ bluetoothAvailable NOTIFY bluetoothAvailableChanged)
    /// Paired devices as a list of { display, address } maps. The first entry is
    /// always the "No device (disabled)" placeholder, whose address is empty.
    Q_PROPERTY(QVariantList devices READ devices NOTIFY devicesChanged)
    /// Human readable status line shown under the paired-phone selector.
    Q_PROPERTY(QString statusText READ statusText NOTIFY statusTextChanged)

public:
    typedef QHash<QString, QVariantMap> InterfaceMap; ///< interface name -> properties
    typedef QHash<QDBusObjectPath, InterfaceMap> ObjectMap; ///< object path -> interfaces

    explicit KCM_MiryuDynamicLock(QObject *parent, const KPluginMetaData &data);
    ~KCM_MiryuDynamicLock() override;

    bool enabled() const;
    void setEnabled(bool value);
    int lockTimeout() const;
    void setLockTimeout(int value);
    int rssiThreshold() const;
    void setRssiThreshold(int value);
    bool cancelOnReconnect() const;
    void setCancelOnReconnect(bool value);
    bool notifyBeforeLock() const;
    void setNotifyBeforeLock(bool value);
    QString deviceAddress() const;
    void setDeviceAddress(const QString &value);
    bool bluetoothAvailable() const;
    QVariantList devices() const;
    QString statusText() const;

Q_SIGNALS:
    // One NOTIFY signal per Q_PROPERTY above. Qt generates static_asserts in the
    // moc output that verify each NOTIFY signal actually exists in the class, so
    // declaring the Q_PROPERTY alone is not enough.
    void enabledChanged();
    void lockTimeoutChanged();
    void rssiThresholdChanged();
    void cancelOnReconnectChanged();
    void notifyBeforeLockChanged();
    void deviceAddressChanged();
    void bluetoothAvailableChanged();
    void devicesChanged();
    void statusTextChanged();

public Q_SLOTS:
    void load() override;
    void save() override;
    void defaults() override;
    /// Re-query BlueZ for the paired-device list and the Bluetooth availability.
    Q_INVOKABLE void refreshDevices();

private:
    /// @return true when BlueZ is reachable on the system bus AND at least one
    /// Bluetooth adapter is present. Drives the no-Bluetooth warning banner.
    bool detectBluetoothAvailable();
    /// Rebuild m_devices from BlueZ, recompute m_statusText and refresh the
    /// cached Bluetooth-availability flag.
    void updateDeviceList();

    bool m_enabled = false;
    int m_lockTimeout = 15;
    int m_rssiThreshold = -75;
    bool m_cancelOnReconnect = true;
    bool m_notifyBeforeLock = true;
    QString m_deviceAddress;
    bool m_bluetoothAvailable = false;
    QVariantList m_devices;
    QString m_statusText;
};

#endif // KCM_MIRYUDYNAMICLOCK_H
