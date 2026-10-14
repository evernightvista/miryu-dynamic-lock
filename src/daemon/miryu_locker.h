/*
    SPDX-License-Identifier: GPL-2.0-or-later
    SPDX-FileCopyrightText: 2026 Miryu Dynamic Lock contributors
*/
#ifndef MIRYU_LOCKER_H
#define MIRYU_LOCKER_H

#include <QObject>
#include <QString>

/**
 * @brief Triggers the KDE screen lock and shows desktop notifications.
 *
 * Locking is attempted through the FreeDesktop ScreenSaver D-Bus interface
 * (implemented by KDE's ksmserver/kscreenlocker in Plasma 6) with a
 * `loginctl lock-session` fallback for environments where the D-Bus call
 * is unavailable.
 */
class MiryuLocker : public QObject
{
    Q_OBJECT
public:
    explicit MiryuLocker(QObject *parent = nullptr);

public Q_SLOTS:
    /// Lock the screen immediately.
    void lockScreen();
    /// Show a desktop notification.
    void notify(const QString &title, const QString &body);

private:
    bool lockViaScreenSaver(const QString &service, const QString &path, const QString &interface);
};

#endif // MIRYU_LOCKER_H
