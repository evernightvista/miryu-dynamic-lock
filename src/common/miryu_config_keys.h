/*
    SPDX-License-Identifier: GPL-2.0-or-later
    SPDX-FileCopyrightText: 2026 Miryu Dynamic Lock contributors
*/
#ifndef MIRYU_CONFIG_KEYS_H
#define MIRYU_CONFIG_KEYS_H

#include <QString>

namespace Miryu
{
// Group name used by both daemon and KCM
inline static const char *const kConfigGroup = "General";

// BlueZ D-Bus constants
inline static const char *const kBluezService = "org.bluez";
inline static const char *const kBluezRootPath = "/";
inline static const char *const kObjManagerIface = "org.freedesktop.DBus.ObjectManager";
inline static const char *const kPropsIface = "org.freedesktop.DBus.Properties";
inline static const char *const kDeviceIface = "org.bluez.Device1";
inline static const char *const kAdapterIface = "org.bluez.Adapter1";

// Screen lock D-Bus constants (freedesktop ScreenSaver, implemented by KDE)
inline static const char *const kScreenSaverService = "org.freedesktop.ScreenSaver";
inline static const char *const kScreenSaverPath = "/org/freedesktop/ScreenSaver";
inline static const char *const kScreenSaverIface = "org.freedesktop.ScreenSaver";
inline static const char *const kScreenSaverInhibit = "Inhibit";
inline static const char *const kScreenSaverLock = "Lock";

// Component / application identifiers
inline static const char *const kApplicationName = "miryu-dynamic-lock";
inline static const char *const kComponentName = "miryu-dynamic-lock";
inline static const char *const kKcmPluginId = "kcm_miryudynamiclock";
} // namespace Miryu

#endif // MIRYU_CONFIG_KEYS_H
