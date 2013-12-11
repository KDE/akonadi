set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${_ENABLE_EXCEPTIONS}")

# Agent server
set(akonadi_agent_server_srcs
  agentpluginloader.cpp
  agentserver.cpp
  agentthread.cpp
  main.cpp
)

add_executable(akonadi_agent_server ${akonadi_agent_server_srcs})

target_link_libraries(akonadi_agent_server
  akonadi_shared
  ${QT_QTGUI_LIBRARIES}
  ${QT_QTDBUS_LIBRARY}
  ${AKONADI_SYSTEM_LIBS}
  ${AKONADI_PROTOCOLINTERNALS_LIBS}
)

target_link_libraries(akonadi_agent_server ${Boost_PROGRAM_OPTIONS_LIBRARY})

# Agent plugin launcher
set(akonadi_agent_launcher_SRCS
  agentpluginloader.cpp
  agentlauncher.cpp
)

add_executable(akonadi_agent_launcher MACOSX_BUNDLE ${akonadi_agent_launcher_SRCS})
target_link_libraries(akonadi_agent_launcher
  akonadi_shared
  ${QT_QTGUI_LIBRARIES}
  ${AKONADI_SYSTEM_LIBS}
  ${AKONADI_PROTOCOLINTERNALS_LIBS}
  ${Boost_PROGRAM_OPTIONS_LIBRARY}
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
        DESTINATION ${BIN_INSTALL_DIR})

