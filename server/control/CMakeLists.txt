include_directories(
  ${CMAKE_BINARY_DIR}/akonadi
  ${QT_QTDBUS_INCLUDE_DIR}
)

set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fexceptions" )

########### next target ###############

set( control_SRCS
  agentmanager.cpp
  main.cpp
  pluginmanager.cpp
  processcontrol.cpp
  profilemanager.cpp
)

qt4_add_dbus_adaptor( control_SRCS ../interfaces/org.kde.Akonadi.AgentManager.xml agentmanager.h AgentManager )
qt4_add_dbus_interfaces( control_SRCS ../interfaces/org.kde.Akonadi.Tracer.xml ../../resources/include/org.kde.Akonadi.Resource.xml)

kde4_automoc( ${control_SRCS} )

kde4_add_executable(akonadi_control ${control_SRCS})
set_target_properties(akonadi_control PROPERTIES OUTPUT_NAME akonadi_control)

target_link_libraries(akonadi_control ${QT_QTCORE_LIBRARIES} ${QT_QTDBUS_LIBRARIES})

install(TARGETS akonadi_control DESTINATION ${BIN_INSTALL_DIR})

