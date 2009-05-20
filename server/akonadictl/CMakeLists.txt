
set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${_ENABLE_EXCEPTIONS}" )
include_directories(${CMAKE_CURRENT_BINARY_DIR})
########### next target ###############

set( akonadictl_SRCS
  ${AKONADI_SHARED_SOURCES}
  akonadistarter.cpp
  main.cpp
)

qt4_add_dbus_interfaces( akonadictl_SRCS
  ${Akonadi_SOURCE_DIR}/server/interfaces/org.freedesktop.Akonadi.ControlManager.xml
)

automoc4_add_executable(akonadictl ${akonadictl_SRCS})
set_target_properties(akonadictl PROPERTIES OUTPUT_NAME akonadictl)

target_link_libraries(akonadictl ${QT_QTCORE_LIBRARY} ${QT_QTDBUS_LIBRARY} ${AKONADI_PROTOCOLINTERNALS_LIBS} ${Boost_PROGRAM_OPTIONS_LIBRARY})

install(TARGETS akonadictl DESTINATION ${BIN_INSTALL_DIR})
