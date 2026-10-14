/*
    SPDX-License-Identifier: GPL-2.0-or-later
    SPDX-FileCopyrightText: 2026 Miryu Dynamic Lock contributors
*/
#ifndef MIRYU_DAEMON_H
#define MIRYU_DAEMON_H

#include <QObject>
#include <QString>
#include <QPointer>

class MiryuBluez;
class MiryuLocker;
class QTimer;
class KConfigWatcher;
typedef QSharedPointer<KConfigWatcher> KConfigWatcherPtr;

/**
 * @brief Core daemon orchestrating Bluetooth monitoring and screen locking.
 *
 * Proximity is judged from two independent signals: the Bluetooth link of the
 * configured paired phone dropping (detected through BlueZ's PropertiesChanged
 * events) and its measured signal strength (RSSI) falling below a configurable
 * threshold. The RSSI check is what lets the lock trigger while the phone is
 * merely a metre or two away, since the link stays up at that distance; the
 * link-drop check remains as the fallback for phones that never report an RSSI.
 *
 * Both signals are re-read on a timer in addition to the D-Bus events, because
 * BlueZ only emits PropertiesChanged when a value actually changes and a phone
 * that slowly drifts out of range may never produce a single event.
 *
 * Once the phone counts as away, a countdown (LockTimeout, default 15 s) starts;
 * when it elapses the daemon asks MiryuLocker to lock the KDE screen. If the
 * phone is back in range before the countdown elapses (and CancelOnReconnect is
 * set), the pending lock is cancelled.
 */
class MiryuDaemon : public QObject
{
    Q_OBJECT
public:
    explicit MiryuDaemon(QObject *parent = nullptr);
    ~MiryuDaemon() override;

public Q_SLOTS:
    /// Reload configuration from disk (called automatically via KConfigWatcher).
    void reloadConfig();

private Q_SLOTS:
    /// Re-evaluate proximity right away after a BlueZ property change.
    void onDeviceStateChanged(const QString &address);
    /// Poll BlueZ for the tracked device's state and update the lock countdown.
    void updateProximity();
    void onLockTimeout();

private:
    void applyInitialState();

    MiryuBluez *m_bluez = nullptr;
    MiryuLocker *m_locker = nullptr;
    QTimer *m_lockTimer = nullptr;
    QTimer *m_pollTimer = nullptr;
    KConfigWatcherPtr m_configWatcher;

    // Cached configuration (re-read from ~/.config/miryu-dynamic-lockrc).
    bool m_enabled = true;
    int m_timeout = 15;
    int m_rssiThreshold = -75;
    QString m_deviceAddress;
    bool m_cancelOnReconnect = true;
    bool m_notifyBeforeLock = true;
    /// Last proximity assessment: true while the phone counts as taken away.
    bool m_away = false;
};

#endif // MIRYU_DAEMON_H
