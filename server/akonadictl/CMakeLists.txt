include_directories(
  ${CMAKE_BINARY_DIR}/akonadi
  ${QT_QTDBUS_INCLUDE_DIR}
)

set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${KDE4_ENABLE_EXCEPTIONS}" )

########### next target ###############

set( akonadictl_SRCS
  main.cpp
)

qt4_add_dbus_interfaces( akonadictl_SRCS
  ../interfaces/org.kde.Akonadi.ControlManager.xml
)

kde4_add_executable(akonadictl NOGUI ${akonadictl_SRCS})
set_target_properties(akonadictl PROPERTIES OUTPUT_NAME akonadictl)

target_link_libraries(akonadictl ${QT_QTCORE_LIBRARY} ${QT_QTDBUS_LIBRARY})

install(TARGETS akonadictl DESTINATION ${BIN_INSTALL_DIR})
