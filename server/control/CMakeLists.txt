include_directories( BEFORE
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_BINARY_DIR}
  ${Boost_INCLUDE_DIR}
)

#find_library( AKONADI_PROTOCOLINTERNALS_LIBRARY NAMES akonadiprotocolinternals
#  PATHS
#  ${CMAKE_LIBRARY_PATH}
#  ${CMAKE_INSTALL_PREFIX}/lib
#)
set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${_ENABLE_EXCEPTIONS}" )

########### next target ###############

set( control_SRCS
  ${AKONADI_SHARED_SOURCES}
  agenttype.cpp
  agentinstance.cpp
  agentmanager.cpp
  controlmanager.cpp
  main.cpp
  processcontrol.cpp
)

qt4_add_dbus_adaptor( control_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/../../interfaces/org.freedesktop.Akonadi.AgentManager.xml agentmanager.h AgentManager )
qt4_add_dbus_adaptor( control_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/../interfaces/org.freedesktop.Akonadi.ControlManager.xml controlmanager.h ControlManager )
qt4_add_dbus_interfaces( control_SRCS
  ${CMAKE_CURRENT_SOURCE_DIR}/../../interfaces/org.freedesktop.Akonadi.Tracer.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/../../interfaces/org.freedesktop.Akonadi.Agent.Control.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/../../interfaces/org.freedesktop.Akonadi.Agent.Status.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/../../interfaces/org.freedesktop.Akonadi.Resource.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/../interfaces/org.freedesktop.Akonadi.Server.xml
)


automoc4_add_executable(akonadi_control ${control_SRCS})
set_target_properties(akonadi_control PROPERTIES OUTPUT_NAME akonadi_control)

target_link_libraries(akonadi_control ${QT_QTCORE_LIBRARY} ${QT_QTDBUS_LIBRARY} ${AKONADI_PROTOCOLINTERNALS_LIBS}  ${Boost_PROGRAM_OPTIONS_LIBRARY})

install(TARGETS akonadi_control DESTINATION ${BIN_INSTALL_DIR})

configure_file(org.freedesktop.Akonadi.Control.service.cmake ${CMAKE_CURRENT_BINARY_DIR}/org.freedesktop.Akonadi.Control.service)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/org.freedesktop.Akonadi.Control.service DESTINATION ${CMAKE_INSTALL_PREFIX}/share/dbus-1/services)
