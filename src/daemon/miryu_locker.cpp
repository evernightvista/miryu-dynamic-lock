/*
    SPDX-License-Identifier: GPL-2.0-or-later
    SPDX-FileCopyrightText: 2026 Miryu Dynamic Lock contributors
*/
#include "miryu_locker.h"
#include "miryu_config_keys.h"

#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusReply>
#include <QProcess>
#include <QVariantList>
#include <QLoggingCategory>

Q_LOGGING_CATEGORY(MIRYU_LOCKER, "miryu.locker")

MiryuLocker::MiryuLocker(QObject *parent)
    : QObject(parent)
{
}

bool MiryuLocker::lockViaScreenSaver(const QString &service, const QString &path, const QString &interface)
{
    QDBusInterface screensaver(service, path, interface, QDBusConnection::sessionBus());
    if (!screensaver.isValid()) {
        return false;
    }
    const QDBusReply<void> reply = screensaver.call(QStringLiteral("Lock"));
    if (!reply.isValid()) {
        qCDebug(MIRYU_LOCKER) << "Lock call on" << service << "failed:" << reply.error().message();
        return false;
    }
    qCInfo(MIRYU_LOCKER) << "Screen locked via" << service;
    return true;
}

void MiryuLocker::lockScreen()
{
    // Primary: the FreeDesktop ScreenSaver interface implemented by KDE.
    if (lockViaScreenSaver(QString::fromLatin1(Miryu::kScreenSaverService),
                           QString::fromLatin1(Miryu::kScreenSaverPath),
                           QString::fromLatin1(Miryu::kScreenSaverIface))) {
        return;
    }
    // Fallback: the legacy KDE screensaver path.
    if (lockViaScreenSaver(QStringLiteral("org.kde.screensaver"),
                           QStringLiteral("/ScreenSaver"),
                           QStringLiteral("org.kde.screensaver"))) {
        return;
    }
    // Last resort: ask logind to lock the caller's session.
    qCInfo(MIRYU_LOCKER) << "Falling back to loginctl lock-session";
    QProcess::startDetached(QStringLiteral("loginctl"), {QStringLiteral("lock-session")});
}

void MiryuLocker::notify(const QString &title, const QString &body)
{
    QDBusInterface notifications(QStringLiteral("org.freedesktop.Notifications"),
                                QStringLiteral("/org/freedesktop/Notifications"),
                                QStringLiteral("org.freedesktop.Notifications"),
                                QDBusConnection::sessionBus());
    if (!notifications.isValid()) {
        return;
    }
    QVariantList args;
    args << QStringLiteral("Miryu Dynamic Lock"); // application name
    args << quint32(0); // replaces id
    args << QStringLiteral("miryu-dynamic-lock"); // icon
    args << title;
    args << body;
    args << QStringList(); // actions
    args << QVariantMap(); // hints
    args << qint32(8000); // timeout (ms)
    notifications.callWithArgumentList(QDBus::NoBlock, QStringLiteral("Notify"), args);
}
