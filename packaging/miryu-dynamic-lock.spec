# SPDX-License-Identifier: GPL-2.0-or-later
# SPDX-FileCopyrightText: 2026 Miryu Dynamic Lock contributors
#
# RPM spec for Miryu Dynamic Lock.
# Builds on Fedora (or compatible) with KDE Plasma 6 / KF6.

%global forgeurl  https://github.com/evernightvista/miryu-dynamic-lock

Name:           miryu-dynamic-lock
Version:        1.0.0
Release:        1%{?dist}
Summary:        Dynamic Bluetooth screen locker for KDE Plasma 6

License:        GPL-2.0-or-later
URL:            %{forgeurl}
Source0:        https://github.com/evernightvista/miryu-dynamic-lock/archive/refs/heads/main.zip
BuildRequires:  cmake
BuildRequires:  gcc-c++
BuildRequires:  extra-cmake-modules >= 6.0
BuildRequires:  qt6-qtbase-devel
BuildRequires:  qt6-qtdeclarative-devel
BuildRequires:  kf6-kcoreaddons-devel
BuildRequires:  kf6-kconfig-devel
BuildRequires:  kf6-kcmutils-devel
BuildRequires:  kf6-ki18n-devel
BuildRequires:  gettext
BuildRequires:  systemd-rpm-macros

Requires:       bluez
Recommends:     (plasma-workspace if plasma-desktop)

# Systemd user unit scriptlets.
%{?systemd_user_ordering}

%description
Miryu Dynamic Lock watches the connection state of a paired Bluetooth phone
through the BlueZ D-Bus daemon and automatically locks the KDE Plasma screen
once the phone is taken out of Bluetooth range, after a configurable delay
(15 seconds by default).

It ships a KDE Plasma 6 KControl module (KCM) that lets you toggle the master
switch, set the lock delay and pick which paired phone to monitor, with
translations for Simplified Chinese, Traditional Chinese, German, Japanese,
French and Korean.

%prep
%autosetup -n %{name}-main

%build
%cmake \
    -DBUILD_TESTING:BOOL=OFF \
    -DCMAKE_INSTALL_LIBDIR:PATH=%{_lib}
%cmake_build

%install
%cmake_install

%find_lang %{name}

%files -f %{name}.lang
%{_bindir}/miryu-dynamic-lock
%{_libdir}/qt6/plugins/plasma/kcms/systemsettings/kcm_miryudynamiclock.so
%{_datadir}/applications/kcm_miryudynamiclock.desktop
%config(noreplace) %{_sysconfdir}/xdg/autostart/miryu-dynamic-lock.desktop
%{_prefix}/lib/systemd/user/miryu-dynamic-lock.service
%{_datadir}/icons/hicolor/scalable/apps/miryu-dynamic-lock.svg

%post
%systemd_user_post %{name}.service

%postun
%systemd_user_postun %{name}.service

%changelog
* Wed Oct 14 2026 KairikiFedora <13278297951@sina.cn> - 1.0.0-1
- Initial packaging of Miryu Dynamic Lock 1.0.0.
