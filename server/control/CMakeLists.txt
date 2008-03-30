include_directories(
  ${CMAKE_BINARY_DIR}/akonadi
  ${QT_QTDBUS_INCLUDE_DIR}
)

find_library( AKONADI_PROTOCOLINTERNALS_LIBRARY NAMES akonadiprotocolinternals
  PATHS
  ${CMAKE_LIBRARY_PATH}
  ${CMAKE_INSTALL_PREFIX}/lib
)
set( AKONADI_PROTOCOLINTERNALS_LIBS ${akonadiprotocolinternals_LIB_DEPENDS} ${AKONADI_PROTOCOLINTERNALS_LIBRARY} )

set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${KDE4_ENABLE_EXCEPTIONS}" )

########### next target ###############

set( control_SRCS
  agentinfo.cpp
  agentmanager.cpp
  controlmanager.cpp
  kcrash.cpp
  main.cpp
  processcontrol.cpp
)

qt4_add_dbus_adaptor( control_SRCS ${KDEPIMLIBS4_DBUS_INTERFACES_DIR}/org.kde.Akonadi.AgentManager.xml agentmanager.h AgentManager )
qt4_add_dbus_adaptor( control_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/../interfaces/org.kde.Akonadi.ControlManager.xml controlmanager.h ControlManager )
qt4_add_dbus_interfaces( control_SRCS
  ${KDEPIMLIBS4_DBUS_INTERFACES_DIR}/org.kde.Akonadi.Tracer.xml
  ${KDEPIMLIBS4_DBUS_INTERFACES_DIR}/org.kde.Akonadi.Agent.Control.xml
  ${KDEPIMLIBS4_DBUS_INTERFACES_DIR}/org.kde.Akonadi.Agent.Status.xml
  ${KDEPIMLIBS4_DBUS_INTERFACES_DIR}/org.kde.Akonadi.Resource.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/../interfaces/org.kde.Akonadi.Server.xml
)


kde4_add_executable(akonadi_control NOGUI ${control_SRCS})
set_target_properties(akonadi_control PROPERTIES OUTPUT_NAME akonadi_control)

target_link_libraries(akonadi_control ${QT_QTCORE_LIBRARY} ${QT_QTDBUS_LIBRARY} ${AKONADI_PROTOCOLINTERNALS_LIBS})

install(TARGETS akonadi_control DESTINATION ${BIN_INSTALL_DIR})

configure_file(org.kde.Akonadi.Control.service.cmake ${CMAKE_CURRENT_BINARY_DIR}/org.kde.Akonadi.Control.service)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/org.kde.Akonadi.Control.service DESTINATION ${CMAKE_INSTALL_PREFIX}/share/dbus-1/services)
