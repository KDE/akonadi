include_directories(
  ${CMAKE_BINARY_DIR}/akonadi
  ${QT_QTDBUS_INCLUDE_DIR}
)

set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${KDE4_ENABLE_EXCEPTIONS}" )

########### next target ###############

set( control_SRCS
  agentinfo.cpp
  agentmanager.cpp
  controlmanager.cpp
  kcrash.cpp
  main.cpp
  processcontrol.cpp
  profilemanager.cpp
  ../../libakonadi/xdgbasedirs.cpp
)

qt4_generate_dbus_interface( ../../libakonadi/agent.h org.kde.Akonadi.Agent.xml )
qt4_generate_dbus_interface( ../../libakonadi/resource.h org.kde.Akonadi.Resource.xml )
qt4_add_dbus_adaptor( control_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/../interfaces/org.kde.Akonadi.AgentManager.xml agentmanager.h AgentManager )
qt4_add_dbus_adaptor( control_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/../interfaces/org.kde.Akonadi.ControlManager.xml controlmanager.h ControlManager )
qt4_add_dbus_adaptor( control_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/../interfaces/org.kde.Akonadi.ProfileManager.xml profilemanager.h ProfileManager )
qt4_add_dbus_interfaces( control_SRCS
  ../interfaces/org.kde.Akonadi.Tracer.xml
  ${CMAKE_CURRENT_BINARY_DIR}/org.kde.Akonadi.Agent.xml
  ${CMAKE_CURRENT_BINARY_DIR}/org.kde.Akonadi.Resource.xml
  ../interfaces/org.kde.Akonadi.Server.xml
)


kde4_add_executable(akonadi_control NOGUI ${control_SRCS})
set_target_properties(akonadi_control PROPERTIES OUTPUT_NAME akonadi_control)

target_link_libraries(akonadi_control ${QT_QTCORE_LIBRARY} ${QT_QTDBUS_LIBRARY})

install(TARGETS akonadi_control DESTINATION ${BIN_INSTALL_DIR})

configure_file(org.kde.Akonadi.Control.service.cmake ${CMAKE_CURRENT_BINARY_DIR}/org.kde.Akonadi.Control.service)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/org.kde.Akonadi.Control.service DESTINATION ${CMAKE_INSTALL_PREFIX}/share/dbus-1/services)
