cmake_minimum_required(VERSION 3.24)

project(miryu-dynamic-lock VERSION 1.0.0 LANGUAGES CXX)

set(KF_MIN_VERSION "6.0.0")
set(QT_MIN_VERSION "6.6.0")
set(PROJECT_VERSION "1.0.0")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

# Bootstrap Extra CMake Modules (ECM). This MUST happen before any include()
# of KDE/ECM modules, otherwise CMAKE_MODULE_PATH does not point at the ECM
# install and the include() calls below fail with "could not find requested
# file: KDEInstallDirs / KDECMakeSettings / KDECompilerSettings / ECMSetupVersion".
#
# The version 6.0 is REQUIRED, not cosmetic: without a version ECM_GLOBAL_FIND_VERSION
# stays empty and ECM's QtVersionOption module falls back to Qt5, so the
# version-agnostic include(KDEInstallDirs) below resolves to KDEInstallDirs5.
# KDEInstallDirs5's plugin dir is ${LIBDIR}/plugins, which installs the KCM to
# /usr/lib64/plugins/... instead of the Qt6 location /usr/lib64/qt6/plugins/...
# that Plasma 6 System Settings scans. Requiring ECM 6.0 (plus including
# KDEInstallDirs6 explicitly below) pins the Qt6/KF6 install-dir scheme.
find_package(ECM 6.0 REQUIRED NO_MODULE)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH} ${CMAKE_MODULE_PATH})

include(FeatureSummary)
# Use the Qt6-specific module directly. With KDE_INSTALL_USE_QT_SYS_PATHS
# (auto-enabled when Qt shares the install prefix, i.e. /usr on Fedora)
# this sets KDE_INSTALL_PLUGINDIR to Qt's own plugin dir, /usr/lib64/qt6/plugins,
# which is exactly where Plasma 6 System Settings expects KCM plugins.
include(KDEInstallDirs6)
include(KDECMakeSettings)
include(KDECompilerSettings NO_POLICY_SCOPE)
include(ECMSetupVersion)

# kcoreaddons_add_plugin() (used in src/kcm/) asserts that
# CMAKE_LIBRARY_OUTPUT_DIRECTORY is set — it writes the plugin .so there during
# the build before installing it. KDECMakeSettings is supposed to define it,
# but on some ECM/build-toolchain combinations (e.g. when the Fedora %cmake
# macro pre-sets CMAKE_LIBRARY_OUTPUT_DIRECTORY to empty, or when a build
# configuration tweak runs after KDECMakeSettings) the variable ends up
# undefined at the point the macro runs, producing:
#   "CMAKE_LIBRARY_OUTPUT_DIRECTORY is not set, set it explicitly or
#    include KDECMakeSettings"
# even though KDECMakeSettings was included. Guard against that by explicitly
# setting the three output directories to the conventional KDE build-tree
# layout whenever they are empty. This is a no-op where KDECMakeSettings
# already did the right thing, and unblocks the macro where it did not.
if(NOT CMAKE_RUNTIME_OUTPUT_DIRECTORY)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
endif()
if(NOT CMAKE_LIBRARY_OUTPUT_DIRECTORY)
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
endif()
if(NOT CMAKE_ARCHIVE_OUTPUT_DIRECTORY)
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
endif()

# Qt6::Qml / Qt6::Quick are required because the KCM is a QtQuick KCM
# (KQuickConfigModule): its UI lives in src/kcm/ui/main.qml, loaded through the
# Qt Quick runtime.
find_package(Qt6 ${QT_MIN_VERSION} CONFIG REQUIRED COMPONENTS
    Core
    DBus
    Gui
    Widgets
    Qml
    Quick
)

# KDE Frameworks 6.
#
# We deliberately call find_package() per framework instead of the umbrella
# form `find_package(KF6 COMPONENTS ...)`. The umbrella form only works when
# ECM ships the FindKF6.cmake find-module, which the KF5-era extra-cmake-modules
# (5.x, e.g. 5.115) does NOT — it only provides FindKF5.cmake. On such a system
# `find_package(KF6 COMPONENTS ...)` falls through to config-mode and dies with
# "Could not find KF6Config.cmake" even when the individual framework -devel
# packages are installed.
#
# Each KF6 framework ships its own KF6FooConfig.cmake, so per-framework calls
# work as long as the framework devel packages are present, regardless of the
# ECM version. They also produce clearer, per-package failure messages.
find_package(KF6CoreAddons ${KF_MIN_VERSION} REQUIRED)
find_package(KF6Config ${KF_MIN_VERSION} REQUIRED)
find_package(KF6KCMUtils ${KF_MIN_VERSION} REQUIRED)
find_package(KF6I18n ${KF_MIN_VERSION} REQUIRED)

set_package_properties(KF6CoreAddons PROPERTIES
    URL "https://api.kde.org/frameworks/kcoreaddons/"
    DESCRIPTION "KDE core addons (KAboutData, KPluginFactory)"
    TYPE REQUIRED
    PURPOSE "Application metadata and the plugin factory used by the KCM.")
set_package_properties(KF6Config PROPERTIES
    URL "https://api.kde.org/frameworks/kconfig/"
    DESCRIPTION "KDE configuration framework (KConfigXT, KConfigWatcher)"
    TYPE REQUIRED
    PURPOSE "Persistent settings (miryu-dynamic-lockrc) and live reload.")
set_package_properties(KF6KCMUtils PROPERTIES
    URL "https://api.kde.org/frameworks/kcmutils/"
    DESCRIPTION "KDE control module utilities (KQuickConfigModule base class)"
    TYPE REQUIRED
    PURPOSE "The Plasma 6 System Settings module.")
set_package_properties(KF6I18n PROPERTIES
    URL "https://api.kde.org/frameworks/ki18n/"
    DESCRIPTION "KDE internationalization framework"
    TYPE REQUIRED
    PURPOSE "Translated UI strings and the ki18n_install() macro.")

find_package(Threads REQUIRED)

# Generate the version header
ecm_setup_version(${PROJECT_VERSION}
    VARIABLE_PREFIX MIRYU
    VERSION_HEADER "${CMAKE_CURRENT_BINARY_DIR}/miryu-version.h"
)

# Shared configuration class (KConfigXT)
add_subdirectory(src/common)

# Daemon binary
add_subdirectory(src/daemon)

# KCM module
add_subdirectory(src/kcm)

# Install data files (autostart, systemd user unit, desktop entries, icons)
add_subdirectory(data)

# Translations: build the .po catalogs in po/<lang>/ into installed .qm files
# under <locale>/<lang>/LC_MESSAGES/miryu-dynamic-lock.qm.
#
# ki18n_install() internally uses find_package(Gettext) to locate msgfmt and
# compile .po -> .qm. If msgfmt is not found, the macro silently skips .qm
# generation and the %files glob in the RPM spec finds no .qm files, failing
# the package build. Declaring Gettext here (rather than relying on the
# macro's internal find_package) makes the failure explicit and ensures
# GETTEXT_MSGFMT_EXECUTABLE is set before ki18n_install runs.
find_package(Gettext REQUIRED)
ki18n_install(po)

feature_summary(WHAT ALL INCLUDE_QUIET_PACKAGES FATAL_ON_MISSING_REQUIRED_PACKAGES)
