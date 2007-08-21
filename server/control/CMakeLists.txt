include_directories(
  ${CMAKE_BINARY_DIR}/akonadi
  ${QT_QTDBUS_INCLUDE_DIR}
)

set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${KDE4_ENABLE_EXCEPTIONS}" )

########### next target ###############

set( control_SRCS
  agentmanager.cpp
  kcrash.cpp
  main.cpp
  processcontrol.cpp
  profilemanager.cpp
  searchprovidermanager.cpp
  ../src/xdgbasedirs.cpp
)

qt4_generate_dbus_interface( ../../libakonadi/resource.h org.kde.Akonadi.Resource.xml )
qt4_add_dbus_adaptor( control_SRCS ../interfaces/org.kde.Akonadi.AgentManager.xml agentmanager.h AgentManager )
qt4_add_dbus_adaptor( control_SRCS ../interfaces/org.kde.Akonadi.ProfileManager.xml profilemanager.h ProfileManager )
qt4_add_dbus_adaptor( control_SRCS ../interfaces/org.kde.Akonadi.SearchProviderManager.xml searchprovidermanager.h Akonadi::SearchProviderManager )
qt4_add_dbus_interfaces( control_SRCS
  ../interfaces/org.kde.Akonadi.Tracer.xml
  ${CMAKE_CURRENT_BINARY_DIR}/org.kde.Akonadi.Resource.xml
  ../../searchproviders/org.kde.Akonadi.SearchProvider.xml
)


kde4_add_executable(akonadi_control NOGUI ${control_SRCS})
set_target_properties(akonadi_control PROPERTIES OUTPUT_NAME akonadi_control)

target_link_libraries(akonadi_control ${QT_QTCORE_LIBRARY} ${QT_QTDBUS_LIBRARY})

install(TARGETS akonadi_control DESTINATION ${BIN_INSTALL_DIR})

configure_file(org.kde.Akonadi.Control.service.cmake ${CMAKE_CURRENT_BINARY_DIR}/org.kde.Akonadi.Control.service)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/org.kde.Akonadi.Control.service DESTINATION ${CMAKE_INSTALL_PREFIX}/share/dbus-1/services)
