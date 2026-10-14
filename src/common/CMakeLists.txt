# Generate the MiryuConfig (KConfigXT) shared class from the .kcfg schema.
# The resulting header/source is consumed by both the daemon and the KCM,
# guaranteeing a single source of truth for the configuration keys and their
# translatable labels/whatsthis.
#
# The library is built STATIC, so no symbol export decoration is needed.

kconfig_add_kcfg_files(miryu_common_config_SRCS
    miryuconfig.kcfgc
    GENERATE_MOC
)

# kconfigskeleton.h — referenced by the KConfigXT-generated miryuconfig.h via
# #include <kconfigskeleton.h> — lives under KF6/KConfigCore/ on KF6. The
# KF6::ConfigCore imported target's INTERFACE_INCLUDE_DIRECTORIES should expose
# that path, but on some distributions (notably Fedora's kf6-kconfig-devel)
# the imported target exposes only the parent KF6/KConfig directory, so the
# bare <kconfigskeleton.h> include fails. Synthesise the correct KConfigCore
# subdirectory from every plausible source variable and add it explicitly.
set(_miryu_kconfig_inc "")
foreach(_candidate
        "${KF6Config_INCLUDE_DIR}"
        "${KF6_INCLUDE_DIR}"
        "${_KF6Config_INCLUDE_DIR}"
        "/usr/include/KF6/KConfig"
        "/usr/include/KF6"
    )
    if(_candidate AND IS_DIRECTORY "${_candidate}")
        list(APPEND _miryu_kconfig_inc "${_candidate}")
    endif()
endforeach()
# The KConfigCore subdirectory is where the skeleton headers actually live.
foreach(_root ${_miryu_kconfig_inc})
    if(IS_DIRECTORY "${_root}/KConfigCore")
        list(APPEND _miryu_kconfig_inc "${_root}/KConfigCore")
    endif()
    if(IS_DIRECTORY "${_root}/KConfigGui")
        list(APPEND _miryu_kconfig_inc "${_root}/KConfigGui")
    endif()
endforeach()
list(REMOVE_DUPLICATES _miryu_kconfig_inc)

add_library(miryu_common_config STATIC ${miryu_common_config_SRCS})
target_include_directories(miryu_common_config PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
    ${_miryu_kconfig_inc}
)

target_link_libraries(miryu_common_config PUBLIC
    Qt6::Core
    # KConfig ships two imported targets — KF6::ConfigCore (KSharedConfig,
    # KConfigGroup, KConfigWatcher, the KConfigSkeleton base of KConfigXT)
    # and KF6::ConfigGui (KConfig widgets). There is NO plain KF6::Config
    # umbrella target, so we link the core target directly. We also link
    # KF6::ConfigGui because kconfigskeleton.h is occasionally installed
    # under the KConfigGui header set on some distributions.
    KF6::ConfigCore
    KF6::ConfigGui
)
