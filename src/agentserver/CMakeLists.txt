# Agent server
set(akonadi_agent_server_srcs
    agentpluginloader.cpp
    agentserver.cpp
    agentthread.cpp
    main.cpp
)

ecm_qt_declare_logging_category(akonadi_agent_server_srcs HEADER akonadiagentserver_debug.h IDENTIFIER AKONADIAGENTSERVER_LOG CATEGORY_NAME org.kde.pim.akonadiagentserver)

add_executable(akonadi_agent_server ${akonadi_agent_server_srcs})

target_link_libraries(akonadi_agent_server
    akonadi_shared
    KF5AkonadiPrivate
    Qt5::Core
    Qt5::DBus
    Qt5::Widgets
)

# Agent plugin launcher
set(akonadi_agent_launcher_SRCS
    agentpluginloader.cpp
    agentlauncher.cpp
    akonadiagentserver_debug.cpp
)

add_executable(akonadi_agent_launcher MACOSX_BUNDLE ${akonadi_agent_launcher_SRCS})

target_link_libraries(akonadi_agent_launcher
    akonadi_shared
    KF5AkonadiPrivate
    Qt5::Core
    Qt5::Widgets
)

if(Q_WS_MAC)
  set_target_properties(akonadi_agent_launcher PROPERTIES MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/../Info.plist.template)
  set_target_properties(akonadi_agent_launcher PROPERTIES MACOSX_BUNDLE_GUI_IDENTIFIER "org.kde.Akonadi.agentlauncher")
  set_target_properties(akonadi_agent_launcher PROPERTIES MACOSX_BUNDLE_BUNDLE_NAME "Akonadi Agent Launcher")
endif()

# Install both helper apps.
if(Q_WS_MAC)
  install(TARGETS akonadi_agent_launcher
          DESTINATION ${AKONADI_BUNDLE_PATH})
else()
  install(TARGETS akonadi_agent_launcher
          DESTINATION ${BIN_INSTALL_DIR})
endif()

install(TARGETS akonadi_agent_server
        ${KF5_INSTALL_TARGETS_DEFAULT_ARGS})
