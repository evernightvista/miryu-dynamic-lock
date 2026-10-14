# The Miryu Dynamic Lock daemon.
#
# It is a headless QCoreApplication that:
#  * monitors the BlueZ D-Bus daemon for the connection state of the
#    configured paired phone, and
#  * triggers the KDE screen lock (org.freedesktop.ScreenSaver) after the
#    configured timeout once the phone is taken away.

add_executable(miryu-dynamic-lock
    main.cpp
    miryu_daemon.cpp
    miryu_bluez.cpp
    miryu_locker.cpp
)

target_link_libraries(miryu-dynamic-lock PRIVATE
    miryu_common_config
    Qt6::Core
    Qt6::DBus
    KF6::ConfigCore
    KF6::CoreAddons
    KF6::I18n
    Threads::Threads
)

install(TARGETS miryu-dynamic-lock DESTINATION ${KDE_INSTALL_BINDIR})
