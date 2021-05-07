set(%{APPNAMELC}resource_SRCS
    %{APPNAMELC}resource.cpp
)

ecm_qt_declare_logging_category(%{APPNAMELC}resource_SRCS
    HEADER debug.h
    IDENTIFIER log_%{APPNAMELC}resource
    CATEGORY_NAME log_%{APPNAMELC}resource
)

kconfig_add_kcfg_files(%{APPNAMELC}resource_SRCS
    ${CMAKE_CURRENT_SOURCE_DIR}/settings.kcfgc
)

kcfg_generate_dbus_interface(
    ${CMAKE_CURRENT_SOURCE_DIR}/settings.kcfg
    org.kde.Akonadi.%{APPNAME}.Settings
)

qt_add_dbus_adaptor(%{APPNAMELC}resource_SRCS
    ${CMAKE_CURRENT_BINARY_DIR}/org.kde.Akonadi.%{APPNAME}.Settings.xml
    ${CMAKE_CURRENT_BINARY_DIR}/settings.h
    Settings
)

add_executable(akonadi_%{APPNAMELC}_resource ${%{APPNAMELC}resource_SRCS})
set_target_properties(akonadi_%{APPNAMELC}_resource PROPERTIES MACOSX_BUNDLE FALSE)

target_link_libraries(akonadi_%{APPNAMELC}_resource
    Qt::DBus
    KF5::AkonadiAgentBase
    KF5::ConfigCore
)

install(TARGETS akonadi_%{APPNAMELC}_resource ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})

install(FILES %{APPNAMELC}resource.desktop
    DESTINATION ${KDE_INSTALL_DATAROOTDIR}/akonadi/agents
)
