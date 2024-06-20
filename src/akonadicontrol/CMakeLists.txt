########### next target ###############
add_executable(akonadi_control)

if(WITH_ACCOUNTS)
    target_sources(akonadi_control PRIVATE accountsintegration.cpp accountsintegration.h)
endif()

ecm_qt_declare_logging_category(akonadi_control HEADER akonadicontrol_debug.h IDENTIFIER AKONADICONTROL_LOG CATEGORY_NAME org.kde.pim.akonadicontrol
        DESCRIPTION "akonadi (Akonadi Control)"
        EXPORT AKONADI
   )

qt_add_dbus_adaptor(control_SRCS ${Akonadi_SOURCE_DIR}/src/interfaces/org.freedesktop.Akonadi.AgentManager.xml agentmanager.h AgentManager)
qt_add_dbus_adaptor(control_SRCS ${Akonadi_SOURCE_DIR}/src/interfaces/org.freedesktop.Akonadi.ControlManager.xml controlmanager.h ControlManager)
qt_add_dbus_adaptor(control_SRCS ${Akonadi_SOURCE_DIR}/src/interfaces/org.freedesktop.Akonadi.AgentManagerInternal.xml agentmanager.h AgentManager)
qt_add_dbus_interfaces(control_SRCS
    ${Akonadi_SOURCE_DIR}/src/interfaces/org.freedesktop.Akonadi.Agent.Control.xml
    ${Akonadi_SOURCE_DIR}/src/interfaces/org.freedesktop.Akonadi.Agent.Status.xml
    ${Akonadi_SOURCE_DIR}/src/interfaces/org.freedesktop.Akonadi.Agent.Search.xml
    ${Akonadi_SOURCE_DIR}/src/interfaces/org.freedesktop.Akonadi.AgentServer.xml
    ${Akonadi_SOURCE_DIR}/src/interfaces/org.freedesktop.Akonadi.Resource.xml
    ${Akonadi_SOURCE_DIR}/src/interfaces/org.freedesktop.Akonadi.Preprocessor.xml
    ${Akonadi_SOURCE_DIR}/src/interfaces/org.freedesktop.Akonadi.Server.xml
    ${Akonadi_SOURCE_DIR}/src/interfaces/org.kde.Akonadi.Accounts.xml
)
qt_add_dbus_interface(control_SRCS ${Akonadi_SOURCE_DIR}/src/interfaces/org.freedesktop.Akonadi.ResourceManager.xml resource_manager)
qt_add_dbus_interface(control_SRCS ${Akonadi_SOURCE_DIR}/src/interfaces/org.freedesktop.Akonadi.PreprocessorManager.xml preprocessor_manager)

target_sources(akonadi_control PRIVATE
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
)
if(COMPILE_WITH_UNITY_CMAKE_SUPPORT)
    set_target_properties(akonadi_control PROPERTIES UNITY_BUILD ON)
endif()

target_include_directories(akonadi_control BEFORE PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>)

set_target_properties(akonadi_control PROPERTIES MACOSX_BUNDLE FALSE)
set_target_properties(akonadi_control PROPERTIES OUTPUT_NAME akonadi_control)

if(WIN32)
    set_target_properties(akonadi_control PROPERTIES WIN32_EXECUTABLE TRUE)
    target_link_libraries(akonadi_control ${QT_QTMAIN_LIBRARY})
endif()

target_link_libraries(akonadi_control
    akonadi_shared
    KPim6::AkonadiPrivate
    KF6::CoreAddons
    KF6::ConfigCore
    Qt::Core
    Qt::DBus
    Qt::Gui
)

if(WITH_ACCOUNTS)
    target_include_directories(akonadi_control PRIVATE ${ACCOUNTSQT_INCLUDE_DIRS})
    # We need Qt::Xml because the Accounts framework leaks QDocument includes into public interface
    target_link_libraries(akonadi_control ${ACCOUNTSQT_LIBRARIES} Qt::Xml)
endif()

install(TARGETS akonadi_control
        ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})

configure_file(org.freedesktop.Akonadi.Control.service.cmake ${CMAKE_CURRENT_BINARY_DIR}/org.freedesktop.Akonadi.Control.service)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/org.freedesktop.Akonadi.Control.service
        DESTINATION ${KDE_INSTALL_DBUSSERVICEDIR})
