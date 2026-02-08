########### next target ###############
add_executable(akonadi_control)

ecm_qt_declare_logging_category(akonadi_control HEADER akonadicontrol_debug.h IDENTIFIER AKONADICONTROL_LOG CATEGORY_NAME org.kde.pim.akonadicontrol
        DESCRIPTION "akonadi (Akonadi Control)"
        EXPORT AKONADI
)

qt_add_dbus_adaptor(control_SRCS ${Akonadi_SOURCE_DIR}/src/interfaces/org.freedesktop.Akonadi.AgentManager.xml agentmanager.h AgentManager)
qt_add_dbus_adaptor(control_SRCS ${Akonadi_SOURCE_DIR}/src/interfaces/org.freedesktop.Akonadi.ControlManager.xml controlmanager.h ControlManager)
qt_add_dbus_adaptor(control_SRCS ${Akonadi_SOURCE_DIR}/src/interfaces/org.freedesktop.Akonadi.AgentManagerInternal.xml agentmanager.h AgentManager)
qt_add_dbus_interfaces(control_SRCS
    ${Akonadi_SOURCE_DIR}/src/interfaces/org.freedesktop.Akonadi.Agent.Account.xml
    ${Akonadi_SOURCE_DIR}/src/interfaces/org.freedesktop.Akonadi.Agent.Control.xml
    ${Akonadi_SOURCE_DIR}/src/interfaces/org.freedesktop.Akonadi.Agent.Status.xml
    ${Akonadi_SOURCE_DIR}/src/interfaces/org.freedesktop.Akonadi.Agent.Search.xml
    ${Akonadi_SOURCE_DIR}/src/interfaces/org.freedesktop.Akonadi.AgentServer.xml
    ${Akonadi_SOURCE_DIR}/src/interfaces/org.freedesktop.Akonadi.Resource.xml
    ${Akonadi_SOURCE_DIR}/src/interfaces/org.freedesktop.Akonadi.Preprocessor.xml
    ${Akonadi_SOURCE_DIR}/src/interfaces/org.freedesktop.Akonadi.Server.xml
)
qt_add_dbus_interface(control_SRCS ${Akonadi_SOURCE_DIR}/src/interfaces/org.freedesktop.Akonadi.ResourceManager.xml resource_manager)
qt_add_dbus_interface(control_SRCS ${Akonadi_SOURCE_DIR}/src/interfaces/org.freedesktop.Akonadi.PreprocessorManager.xml preprocessor_manager)

target_sources(
    akonadi_control
    PRIVATE
        agenttype.cpp
        agentinstance.cpp
        agentbrokeninstance.cpp
        agentprocessinstance.cpp
        agentthreadinstance.cpp
        agentmanager.cpp
        controlmanager.cpp
        processcontrol.cpp
        main.cpp
        ${control_SRCS}
        agenttype.h
        agentinstance.h
        agentbrokeninstance.h
        agentprocessinstance.h
        agentthreadinstance.h
        agentmanager.h
        controlmanager.h
        processcontrol.h
        onlineaccountsintegration.cpp
        onlineaccountsintegration.h
)
if(COMPILE_WITH_UNITY_CMAKE_SUPPORT)
    set_target_properties(
        akonadi_control
        PROPERTIES
            UNITY_BUILD
                ON
    )
endif()

target_include_directories(akonadi_control BEFORE PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>)

set_target_properties(
    akonadi_control
    PROPERTIES
        MACOSX_BUNDLE
            FALSE
)
set_target_properties(
    akonadi_control
    PROPERTIES
        OUTPUT_NAME
            akonadi_control
)

if(WIN32)
    set_target_properties(
        akonadi_control
        PROPERTIES
            WIN32_EXECUTABLE
                TRUE
    )
    target_link_libraries(akonadi_control ${QT_QTMAIN_LIBRARY})
endif()

target_link_libraries(
    akonadi_control
    akonadi_shared
    KPim6::AkonadiPrivate
    KF6::CoreAddons
    KF6::ConfigCore
    Qt::Core
    Qt::DBus
    Qt::Gui
)

install(
    TARGETS
        akonadi_control
        ${KDE_INSTALL_TARGETS_DEFAULT_ARGS}
)

ecm_generate_dbus_service_file(
    NAME org.freedesktop.Akonadi.Control
    EXECUTABLE "${KDE_INSTALL_FULL_BINDIR}/akonadi_control"
    SYSTEMD_SERVICE akonadi_control.service
    DESTINATION ${KDE_INSTALL_DBUSSERVICEDIR}
)

ecm_install_configured_files(
    INPUT akonadi_control.service.in
    @ONLY
    DESTINATION ${KDE_INSTALL_SYSTEMDUSERUNITDIR}
)
