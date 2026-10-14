# KDE Plasma 6 KCM for Miryu Dynamic Lock.
#
# Built as a QtQuick KCM. kcmutils_add_qml_kcm() wraps kcoreaddons_add_plugin()
# with the default INSTALL_NAMESPACE "plasma/kcms/systemsettings", which is the
# namespace Plasma 6 System Settings scans for QML-based KCMs, and additionally:
#   * bundles everything under src/kcm/ui/ into a qrc at /kcm/<target>/ ...
#     (the resource prefix KQuickConfigModule expects), and
#   * generates the companion .desktop file from the JSON metadata so the module
#     also shows up in KRunner / kmenu and can be launched standalone.
#
# A QML KCM is what makes the header switch possible: the QML root element
# exposes actions[] and System Settings renders a checkable Kirigami.Action
# (displayComponent: QQC2.Switch) to the right of the page title. A QWidget
# KCModule is rendered with a plain text title only.

kcmutils_add_qml_kcm(kcm_miryudynamiclock)

target_sources(kcm_miryudynamiclock PRIVATE
    kcm_miryudynamiclock.cpp
)

target_link_libraries(kcm_miryudynamiclock PRIVATE
    miryu_common_config
    Qt6::Core
    Qt6::Gui
    Qt6::Qml
    Qt6::Quick
    Qt6::DBus
    KF6::CoreAddons
    KF6::ConfigCore
    KF6::KCMUtils
    KF6::I18n
)

# AUTOMOC writes the generated kcm_miryudynamiclock.moc into the build tree's
# corresponding subdirectory; the explicit #include "kcm_miryudynamiclock.moc"
# at the bottom of the .cpp therefore needs both directories on the include
# path.
target_include_directories(kcm_miryudynamiclock PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
)
