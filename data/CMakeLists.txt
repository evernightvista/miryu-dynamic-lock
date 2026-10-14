# Data files shipped with Miryu Dynamic Lock.

# Autostart entry: a standard XDG autostart .desktop file installed under
# /etc/xdg/autostart so it is started for every XDG-compliant session,
# including KDE Plasma.
install(FILES miryu-dynamic-lock.desktop
        DESTINATION "${CMAKE_INSTALL_FULL_SYSCONFDIR}/xdg/autostart")

# systemd user unit. The .in file is configured by CMake so the daemon's
# absolute install path is substituted in at configure time. Installed to the
# lib-based systemd user unit directory that systemd actually scans
# (/usr/lib/systemd/user on Fedora-style layouts).
configure_file(miryu-dynamic-lock.service.in
    ${CMAKE_CURRENT_BINARY_DIR}/miryu-dynamic-lock.service
    @ONLY)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/miryu-dynamic-lock.service
        DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/systemd/user")

# Application icon (scalable). Registers the icon name "miryu-dynamic-lock"
# in the hicolor theme so the desktop entry and KCM both resolve it.
install(FILES icons/miryu-dynamic-lock.svg
        DESTINATION ${KDE_INSTALL_ICONDIR}/hicolor/scalable/apps)
