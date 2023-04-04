########### next target ###############


add_executable(akonadictl)
ecm_qt_declare_logging_category(akonadictl HEADER akonadictl_debug.h IDENTIFIER AKONADICTL_LOG CATEGORY_NAME org.kde.pim.akonadictl
        DESCRIPTION "akonadi (Akonadi Control)"
        OLD_CATEGORY_NAMES log_akonadictl
        EXPORT AKONADI
    )


qt_add_dbus_interfaces(akonadictl_SRCS
    ${Akonadi_SOURCE_DIR}/src/interfaces/org.freedesktop.Akonadi.ControlManager.xml
    ${Akonadi_SOURCE_DIR}/src/interfaces/org.freedesktop.Akonadi.Janitor.xml
)
target_sources(akonadictl PRIVATE ${akonadictl_SRCS}
    akonadistarter.cpp
    main.cpp
    akonadistarter.h
)

if (COMPILE_WITH_UNITY_CMAKE_SUPPORT)
    set_target_properties(akonadictl PROPERTIES UNITY_BUILD ON)
endif()

set_target_properties(akonadictl PROPERTIES MACOSX_BUNDLE FALSE)
set_target_properties(akonadictl PROPERTIES OUTPUT_NAME akonadictl)
ecm_mark_nongui_executable(akonadictl)
target_link_libraries(akonadictl
    akonadi_shared
    KPim6::AkonadiPrivate
    KF6::CoreAddons
    Qt::Core
    Qt::DBus
)

install(TARGETS akonadictl
        ${KDE_INSTALL_TARGETS_DEFAULT_ARGS}
)
