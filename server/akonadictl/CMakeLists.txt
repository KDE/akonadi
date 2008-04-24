include_directories(
  ${CMAKE_BINARY_DIR}/akonadi
  ${QT_QTDBUS_INCLUDE_DIR}
)

set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${_ENABLE_EXCEPTIONS}" )
include_directories(${CMAKE_CURRENT_BINARY_DIR})
########### next target ###############

set( akonadictl_SRCS
  akonadistarter.cpp
  main.cpp
)

qt4_automoc(${akonadictl_SRCS})

qt4_add_dbus_interfaces( akonadictl_SRCS
  ${CMAKE_CURRENT_SOURCE_DIR}/../interfaces/org.kde.Akonadi.ControlManager.xml
)

add_executable(akonadictl ${akonadictl_SRCS})
set_target_properties(akonadictl PROPERTIES OUTPUT_NAME akonadictl)

target_link_libraries(akonadictl ${QT_QTCORE_LIBRARY} ${QT_QTDBUS_LIBRARY})

install(TARGETS akonadictl DESTINATION ${BIN_INSTALL_DIR})
