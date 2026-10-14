/*
    SPDX-License-Identifier: GPL-2.0-or-later
    SPDX-FileCopyrightText: 2026 Miryu Dynamic Lock contributors
*/
#include "miryu_daemon.h"
#include "miryu_config_keys.h"

#include <QCoreApplication>
#include <QDBusConnection>
#include <QLoggingCategory>

#include <KAboutData>
#include <KLocalizedString>

Q_LOGGING_CATEGORY(MIRYU_APP, "miryu.app")

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    app.setApplicationName(QString::fromLatin1(Miryu::kApplicationName));
    // QCoreApplication::setOrganizationDomain takes a QString. Use
    // QStringLiteral (not QByteArrayLiteral) because QT_NO_CAST_FROM_BYTEARRAY
    // is enabled, which deletes QString's implicit QByteArray constructor.
    app.setOrganizationDomain(QStringLiteral("miryu.org"));

    KLocalizedString::setApplicationDomain(QByteArrayLiteral("miryu-dynamic-lock"));

    KAboutData about(QStringLiteral("miryu-dynamic-lock"),
                     i18n("Miryu Dynamic Lock"),
                     QStringLiteral("1.0.0"),
                     i18n("Automatically lock the KDE screen when the paired phone is taken away."),
                     // The KF5 enum KAboutLicense::GPL_V2_PLUS was removed in
                     // KF6. The KF6 way to express "GPL-2.0-or-later" is to
                     // pass GPL_V2 as the base license here, then apply the
                     // OrLaterVersions restriction via setLicense() below.
                     KAboutLicense::GPL_V2,
                     QStringLiteral("© 2026 Miryu Dynamic Lock contributors"),
                     i18n("Dynamic Bluetooth screen locker for KDE Plasma."),
                     QStringLiteral("https://invent.kde.org/miryu/dynamic-lock"));
    // Upgrade the license to "GPL-2.0-or-later" (SPDX).
    about.setLicense(KAboutLicense::GPL_V2, KAboutLicense::OrLaterVersions);
    // The user-visible display name belongs on the KAboutData, not on the
    // QCoreApplication (which in Qt6 does not have setApplicationDisplayName
    // — that method only exists on QGuiApplication, and the daemon is a
    // headless QCoreApplication).
    about.setDisplayName(i18n("Miryu Dynamic Lock"));
    about.setDesktopFileName(QStringLiteral("miryu-dynamic-lock"));
    // NOTE: KAboutData::setOrganizationDomain takes a QByteArray (unlike
    // QCoreApplication::setOrganizationDomain above, which takes a QString),
    // so QByteArrayLiteral is the correct literal here.
    about.setOrganizationDomain(QByteArrayLiteral("miryu.org"));
    KAboutData::setApplicationData(about);

    // Ensure only one instance of the daemon is running. If a second instance
    // is launched (e.g. after a re-login or via systemd activation) it exits
    // quietly, leaving the already-running instance in charge.
    const QString serviceName = QStringLiteral("org.miryu.DynamicLock");
    if (!QDBusConnection::sessionBus().registerService(serviceName)) {
        qCInfo(MIRYU_APP) << "Another instance is already running (" << serviceName
                          << "); exiting.";
        return 0;
    }

    MiryuDaemon daemon;
    return app.exec();
}
