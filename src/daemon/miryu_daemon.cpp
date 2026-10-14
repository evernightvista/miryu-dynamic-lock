/*
    SPDX-License-Identifier: GPL-2.0-or-later
    SPDX-FileCopyrightText: 2026 Miryu Dynamic Lock contributors
*/
#include "miryu_daemon.h"
#include "miryu_bluez.h"
#include "miryu_locker.h"
#include "miryu_config_keys.h"

#include <KConfig>
#include <KConfigGroup>
#include <KConfigWatcher>
#include <KSharedConfig>
#include <KLocalizedString>

#include <QTimer>
#include <QLoggingCategory>

Q_LOGGING_CATEGORY(MIRYU_DAEMON, "miryu.daemon")

namespace
{
// How often the daemon re-reads the tracked device's state, in milliseconds.
// BlueZ only emits PropertiesChanged when a value actually changes, so polling
// is what catches a phone that slowly drifts out of range without ever
// producing a link drop, and what recovers from a missed event.
constexpr int kPollIntervalMs = 5000;

// RSSI is expressed in dBm and becomes more negative the farther the phone is
// (about -40 next to the PC, -85 across a room). The configured threshold is
// the value below which the phone counts as away. When coming back, a slightly
// higher value is required, so a signal hovering around the threshold cannot
// make the lock countdown start and stop endlessly.
constexpr int kRssiHysteresisDbm = 8;

// Combine both proximity signals: the phone counts as away once the Bluetooth
// link drops, and otherwise once its signal strength falls below the threshold.
// A connected device that does not report an RSSI (typical for classic/BR-EDR
// phones, whose RSSI BlueZ only knows while scanning) keeps the previous
// assessment instead of guessing, leaving the link state as the deciding signal.
bool isAwayFrom(bool connected, bool rssiValid, int rssi, int threshold, bool wasAway)
{
    if (!connected) {
        return true;
    }
    if (!rssiValid) {
        return wasAway;
    }
    if (wasAway) {
        return rssi < threshold + kRssiHysteresisDbm;
    }
    return rssi < threshold;
}
} // namespace

MiryuDaemon::MiryuDaemon(QObject *parent)
    : QObject(parent)
    , m_bluez(new MiryuBluez(this))
    , m_locker(new MiryuLocker(this))
    , m_lockTimer(new QTimer(this))
    , m_pollTimer(new QTimer(this))
{
    m_lockTimer->setSingleShot(true);
    connect(m_lockTimer, &QTimer::timeout, this, &MiryuDaemon::onLockTimeout);

    // Periodic re-read of the tracked device's state, in addition to the D-Bus
    // events (see kPollIntervalMs).
    m_pollTimer->setInterval(kPollIntervalMs);
    connect(m_pollTimer, &QTimer::timeout, this, &MiryuDaemon::updateProximity);

    connect(m_bluez, &MiryuBluez::deviceStateChanged,
            this, &MiryuDaemon::onDeviceStateChanged);

    // Live-reload configuration when the KCM (or anything else) writes to the
    // config file. KConfigWatcher uses KDirWatch/inotify so this works across
    // processes.
    m_configWatcher = KConfigWatcher::create(KSharedConfig::openConfig(QStringLiteral("miryu-dynamic-lockrc")));
    connect(m_configWatcher.data(), &KConfigWatcher::configChanged, this,
            [this](const KConfigGroup &group, const QByteArrayList &) {
                if (group.name() == QLatin1String(Miryu::kConfigGroup)) {
                    reloadConfig();
                }
            });

    reloadConfig();
    applyInitialState();
}

MiryuDaemon::~MiryuDaemon() = default;

void MiryuDaemon::reloadConfig()
{
    KSharedConfigPtr cfg = KSharedConfig::openConfig(QStringLiteral("miryu-dynamic-lockrc"));
    const KConfigGroup group(cfg, QLatin1String(Miryu::kConfigGroup));

    // Use QStringLiteral for the keys: QT_NO_CAST_FROM_ASCII deletes QString's
    // implicit const char* constructor, so bare literals do not compile.
    const bool newEnabled = group.readEntry(QStringLiteral("Enabled"), true);
    const int newTimeout = group.readEntry(QStringLiteral("LockTimeout"), 15);
    const QString newDevice = group.readEntry(QStringLiteral("DeviceAddress"), QString()).toUpper();
    const bool newCancel = group.readEntry(QStringLiteral("CancelOnReconnect"), true);
    const bool newNotify = group.readEntry(QStringLiteral("NotifyBeforeLock"), true);
    const int newRssiThreshold = group.readEntry(QStringLiteral("RssiThreshold"), -75);

    const bool deviceChanged = (newDevice != m_deviceAddress);
    m_enabled = newEnabled;
    m_timeout = newTimeout;
    m_cancelOnReconnect = newCancel;
    m_notifyBeforeLock = newNotify;
    m_rssiThreshold = newRssiThreshold;

    if (deviceChanged) {
        m_deviceAddress = newDevice;
        m_lockTimer->stop();
        // Forget the proximity of the previous device; the next evaluation
        // re-reads it from BlueZ.
        m_away = false;
        qCInfo(MIRYU_DAEMON) << "Tracking device" << m_deviceAddress;
    }

    if (m_enabled && !m_deviceAddress.isEmpty()) {
        // The countdown itself is left to the next evaluation (the poll fires
        // within kPollIntervalMs), so merely enabling the feature never locks
        // the screen by itself.
        if (!m_pollTimer->isActive()) {
            m_pollTimer->start();
        }
    } else {
        m_pollTimer->stop();
        m_lockTimer->stop();
        if (!m_enabled) {
            qCInfo(MIRYU_DAEMON) << "Dynamic lock is disabled in settings.";
        }
    }
}

void MiryuDaemon::applyInitialState()
{
    if (!m_enabled || m_deviceAddress.isEmpty()) {
        return;
    }

    // Record the current proximity without arming the countdown: a lock must not
    // fire merely because the daemon started while the phone was already away.
    MiryuBluez::BluezDevice device;
    if (m_bluez->findDevice(m_deviceAddress, device)) {
        m_away = isAwayFrom(device.connected, device.rssiValid, device.rssi, m_rssiThreshold, m_away);
        qCInfo(MIRYU_DAEMON) << "Initial state for" << m_deviceAddress
                             << "connected:" << device.connected
                             << "rssi:" << (device.rssiValid ? QString::number(device.rssi) : QStringLiteral("n/a"))
                             << "away:" << m_away;
    }
}

void MiryuDaemon::onDeviceStateChanged(const QString &address)
{
    if (address.toUpper() != m_deviceAddress.toUpper()) {
        return; // a device we do not track
    }
    // React immediately instead of waiting for the next poll.
    updateProximity();
}

void MiryuDaemon::updateProximity()
{
    if (!m_enabled || m_deviceAddress.isEmpty()) {
        return;
    }

    MiryuBluez::BluezDevice device;
    if (!m_bluez->findDevice(m_deviceAddress, device)) {
        // BlueZ does not know the device right now (e.g. it just restarted);
        // keep the previous assessment rather than locking on a guess.
        return;
    }

    const bool away = isAwayFrom(device.connected, device.rssiValid, device.rssi, m_rssiThreshold, m_away);
    if (away == m_away) {
        return;
    }
    m_away = away;

    if (away) {
        // The phone was taken away: arm the lock countdown.
        m_lockTimer->start(m_timeout * 1000);
        qCInfo(MIRYU_DAEMON) << "Phone is away (connected:" << device.connected
                             << "rssi:" << (device.rssiValid ? QString::number(device.rssi) : QStringLiteral("n/a"))
                             << "); locking screen in" << m_timeout << "s.";
        if (m_notifyBeforeLock) {
            m_locker->notify(i18nc("@info:notification", "Phone disconnected"),
                             i18ncp("@info:notification",
                                    "Locking the screen in %1 second.",
                                    "Locking the screen in %1 seconds.",
                                    m_timeout));
        }
    } else if (m_cancelOnReconnect && m_lockTimer->isActive()) {
        // The phone came back before the countdown elapsed.
        m_lockTimer->stop();
        qCInfo(MIRYU_DAEMON) << "Phone is back in range; pending screen lock cancelled.";
        if (m_notifyBeforeLock) {
            m_locker->notify(i18nc("@info:notification", "Phone reconnected"),
                             i18nc("@info:notification", "Pending screen lock cancelled."));
        }
    }
}

void MiryuDaemon::onLockTimeout()
{
    qCInfo(MIRYU_DAEMON) << "Lock timeout elapsed; locking the screen.";
    m_locker->lockScreen();
}
