/*
    SPDX-License-Identifier: GPL-2.0-or-later
    SPDX-FileCopyrightText: 2026 Miryu Dynamic Lock contributors
*/
#include "kcm_miryudynamiclock.h"
#include "miryu_config_keys.h"
#include "miryuconfig.h"

#include <KPluginFactory>
#include <KLocalizedString>

#include <QDBusArgument>
#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QDBusInterface>
#include <QDBusMetaType>
#include <QDBusReply>
#include <QDBusVariant>
#include <QDebug>
#include <QProcess>
#include <QStringList>

K_PLUGIN_CLASS_WITH_JSON(KCM_MiryuDynamicLock, "kcm_miryudynamiclock.json")

KCM_MiryuDynamicLock::KCM_MiryuDynamicLock(QObject *parent, const KPluginMetaData &data)
    : KQuickConfigModule(parent, data)
{
    qDBusRegisterMetaType<InterfaceMap>();
    qDBusRegisterMetaType<ObjectMap>();

    // NOTE: We intentionally do NOT call KLocalizedString::setApplicationDomain()
    // here. KCM plugins run inside the systemsettings process; changing the
    // global translation domain would break translation of systemsettings' own
    // UI (e.g. the unsaved-changes warning dialog). The QML side therefore uses
    // i18nd()/i18ndc() with the explicit "miryu-dynamic-lock" domain, and this
    // C++ side keeps using i18ndc()/i18ndcp() the same way.

    // load() is invoked by the module container right after construction, so the
    // QML reads the stored values as soon as it is created.
}

KCM_MiryuDynamicLock::~KCM_MiryuDynamicLock() = default;

bool KCM_MiryuDynamicLock::enabled() const
{
    return m_enabled;
}

void KCM_MiryuDynamicLock::setEnabled(bool value)
{
    if (value == m_enabled) {
        return;
    }
    m_enabled = value;
    Q_EMIT enabledChanged();
    setNeedsSave(true);
}

int KCM_MiryuDynamicLock::lockTimeout() const
{
    return m_lockTimeout;
}

void KCM_MiryuDynamicLock::setLockTimeout(int value)
{
    if (value == m_lockTimeout) {
        return;
    }
    m_lockTimeout = value;
    Q_EMIT lockTimeoutChanged();
    setNeedsSave(true);
}

int KCM_MiryuDynamicLock::rssiThreshold() const
{
    return m_rssiThreshold;
}

void KCM_MiryuDynamicLock::setRssiThreshold(int value)
{
    if (value == m_rssiThreshold) {
        return;
    }
    m_rssiThreshold = value;
    Q_EMIT rssiThresholdChanged();
    setNeedsSave(true);
}

bool KCM_MiryuDynamicLock::cancelOnReconnect() const
{
    return m_cancelOnReconnect;
}

void KCM_MiryuDynamicLock::setCancelOnReconnect(bool value)
{
    if (value == m_cancelOnReconnect) {
        return;
    }
    m_cancelOnReconnect = value;
    Q_EMIT cancelOnReconnectChanged();
    setNeedsSave(true);
}

bool KCM_MiryuDynamicLock::notifyBeforeLock() const
{
    return m_notifyBeforeLock;
}

void KCM_MiryuDynamicLock::setNotifyBeforeLock(bool value)
{
    if (value == m_notifyBeforeLock) {
        return;
    }
    m_notifyBeforeLock = value;
    Q_EMIT notifyBeforeLockChanged();
    setNeedsSave(true);
}

QString KCM_MiryuDynamicLock::deviceAddress() const
{
    return m_deviceAddress;
}

void KCM_MiryuDynamicLock::setDeviceAddress(const QString &value)
{
    if (value == m_deviceAddress) {
        return;
    }
    m_deviceAddress = value;
    Q_EMIT deviceAddressChanged();
    setNeedsSave(true);
}

bool KCM_MiryuDynamicLock::bluetoothAvailable() const
{
    return m_bluetoothAvailable;
}

QVariantList KCM_MiryuDynamicLock::devices() const
{
    return m_devices;
}

QString KCM_MiryuDynamicLock::statusText() const
{
    return m_statusText;
}

bool KCM_MiryuDynamicLock::detectBluetoothAvailable()
{
    // Two independent conditions must hold:
    //  1. The BlueZ daemon is registered on the system D-Bus as org.bluez
    //     (otherwise we cannot enumerate adapters or devices).
    //  2. At least one Bluetooth adapter is exported by BlueZ (an adapter can
    //     exist in software via BlueZ even when the radio is powered off; what
    //     matters here is that the host has a Bluetooth stack at all).
    QDBusConnection bus = QDBusConnection::systemBus();
    if (!bus.isConnected()) {
        return false;
    }

    QDBusReply<QString> serviceReply =
        bus.interface()->serviceOwner(QString::fromLatin1(Miryu::kBluezService));
    if (!serviceReply.isValid() || serviceReply.value().isEmpty()) {
        return false;
    }

    QDBusInterface objManager(QString::fromLatin1(Miryu::kBluezService),
                              QString::fromLatin1(Miryu::kBluezRootPath),
                              QString::fromLatin1(Miryu::kObjManagerIface),
                              bus);
    if (!objManager.isValid()) {
        return false;
    }
    const QDBusReply<ObjectMap> reply = objManager.call(QStringLiteral("GetManagedObjects"));
    if (!reply.isValid()) {
        return false;
    }
    const ObjectMap objects = reply.value();
    for (auto it = objects.constBegin(); it != objects.constEnd(); ++it) {
        const InterfaceMap &ifaces = it.value();
        if (ifaces.contains(QString::fromLatin1(Miryu::kAdapterIface))) {
            return true;
        }
    }
    return false;
}

void KCM_MiryuDynamicLock::updateDeviceList()
{
    m_bluetoothAvailable = detectBluetoothAvailable();

    QVariantList list;

    // The first entry is the "disabled" placeholder so that the combo box index
    // 0 always means "no device monitored" (matching the stored empty address).
    QVariantMap none;
    none.insert(QStringLiteral("display"),
                i18ndc("miryu-dynamic-lock", "@item:inlistbox", "No device (disabled)"));
    none.insert(QStringLiteral("address"), QString());
    list.append(none);

    if (!m_bluetoothAvailable) {
        m_statusText = i18ndc("miryu-dynamic-lock", "@info:status",
            "Bluetooth is not available on this computer.");
    } else {
        QDBusInterface objManager(QString::fromLatin1(Miryu::kBluezService),
                                  QString::fromLatin1(Miryu::kBluezRootPath),
                                  QString::fromLatin1(Miryu::kObjManagerIface),
                                  QDBusConnection::systemBus());
        if (!objManager.isValid()) {
            m_statusText = i18ndc("miryu-dynamic-lock", "@info:status",
                "BlueZ D-Bus service is not available.");
        } else {
            const QDBusReply<ObjectMap> reply = objManager.call(QStringLiteral("GetManagedObjects"));
            if (!reply.isValid()) {
                m_statusText = i18ndc("miryu-dynamic-lock", "@info:status",
                    "Could not query BlueZ: %1", reply.error().message());
            } else {
                const ObjectMap objects = reply.value();
                int paired = 0;
                for (auto it = objects.constBegin(); it != objects.constEnd(); ++it) {
                    const InterfaceMap &ifaces = it.value();
                    if (!ifaces.contains(QString::fromLatin1(Miryu::kDeviceIface))) {
                        continue;
                    }
                    const QVariantMap props = ifaces.value(QString::fromLatin1(Miryu::kDeviceIface));
                    if (!props.value(QStringLiteral("Paired")).toBool()) {
                        continue;
                    }
                    ++paired;
                    const QString address = props.value(QStringLiteral("Address")).toString().toUpper();
                    const QString name = props.value(QStringLiteral("Name")).toString();
                    const QString alias = props.value(QStringLiteral("Alias")).toString();
                    const bool connected = props.value(QStringLiteral("Connected")).toBool();
                    const QString display = (name.isEmpty() ? alias : name)
                                            + QStringLiteral(" (") + address + QStringLiteral(")")
                                            + (connected ? i18ndc("miryu-dynamic-lock",
                                                                   "@info:inlistbox appended status",
                                                                   " — connected")
                                                         : QString());
                    QVariantMap device;
                    device.insert(QStringLiteral("display"), display);
                    device.insert(QStringLiteral("address"), address);
                    list.append(device);
                }

                if (paired == 0) {
                    m_statusText = i18ndc("miryu-dynamic-lock", "@info:status",
                        "No paired Bluetooth devices found. Pair a device in the KDE "
                        "Bluetooth settings first.");
                } else {
                    m_statusText = i18ndcp("miryu-dynamic-lock", "@info:status",
                        "%1 paired device found.",
                        "%1 paired devices found.",
                        paired);
                }
            }
        }
    }

    m_devices = list;
    Q_EMIT devicesChanged();
    Q_EMIT statusTextChanged();
    Q_EMIT bluetoothAvailableChanged();
}

void KCM_MiryuDynamicLock::refreshDevices()
{
    updateDeviceList();
}

void KCM_MiryuDynamicLock::load()
{
    MiryuConfig *cfg = MiryuConfig::self();
    setEnabled(cfg->enabled());
    setLockTimeout(cfg->lockTimeout());
    setRssiThreshold(cfg->rssiThreshold());
    setCancelOnReconnect(cfg->cancelOnReconnect());
    setNotifyBeforeLock(cfg->notifyBeforeLock());
    setDeviceAddress(cfg->deviceAddress().toUpper());

    // Query BlueZ after the stored address is in place so the QML can select it.
    updateDeviceList();

    // The setters above flag the module as modified; loading must not count as a
    // user edit, otherwise System Settings would show its "unsaved changes"
    // dialog on the next page switch even though the user changed nothing.
    setNeedsSave(false);
}

void KCM_MiryuDynamicLock::save()
{
    MiryuConfig *cfg = MiryuConfig::self();
    cfg->setEnabled(m_enabled);
    cfg->setLockTimeout(m_lockTimeout);
    cfg->setRssiThreshold(m_rssiThreshold);
    cfg->setDeviceAddress(m_deviceAddress);
    cfg->setCancelOnReconnect(m_cancelOnReconnect);
    cfg->setNotifyBeforeLock(m_notifyBeforeLock);
    cfg->save();

    // Bring the systemd user unit in line with the master switch. This runs
    // here — that is, only once the user confirms with Apply/OK and not while
    // the switch is merely being toggled — so that pressing Cancel really
    // leaves the service untouched. "enable --now" starts the daemon right away
    // and from every following login onwards; "disable --now" stops it
    // immediately and keeps it from starting at the next login.
    const QStringList systemctlArgs{
        QStringLiteral("--user"),
        m_enabled ? QStringLiteral("enable") : QStringLiteral("disable"),
        QStringLiteral("--now"),
        QStringLiteral("miryu-dynamic-lock.service"),
    };
    if (!QProcess::startDetached(QStringLiteral("systemctl"), systemctlArgs)) {
        qWarning() << "Miryu Dynamic Lock: could not run systemctl" << systemctlArgs;
    }

    setNeedsSave(false);
}

void KCM_MiryuDynamicLock::defaults()
{
    setEnabled(false);
    setLockTimeout(15);
    setRssiThreshold(-75);
    setCancelOnReconnect(true);
    setNotifyBeforeLock(true);
    setDeviceAddress(QString());
    updateDeviceList();
    setNeedsSave(true);
}

// AutoMoc requires the explicit moc include for files containing
// K_PLUGIN_CLASS_WITH_JSON — it detects the macro and refuses to generate
// the moc compilation unit automatically. The .moc file is produced by
// AUTOMOC into the build tree and resolved via the source-relative include.
#include "kcm_miryudynamiclock.moc"
