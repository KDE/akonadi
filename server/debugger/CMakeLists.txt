include_directories( ${QT_QTDBUS_INCLUDE_DIR} )

set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fexceptions" )

########### next target ###############

set( debugger_SRCS
  connectionpage.cpp
  main.cpp
  mainwindow.cpp
)

qt4_add_dbus_interfaces( debugger_SRCS
  ../interfaces/org.kde.Akonadi.TracerNotification.xml
)

kde4_automoc( ${debugger_SRCS} )

kde4_add_executable(akonadi_debugger ${debugger_SRCS})
set_target_properties(akonadi_debugger PROPERTIES OUTPUT_NAME akonadi_debugger)

target_link_libraries(akonadi_debugger ${QT_QTCORE_LIBRARIES} ${QT_QTGUI_LIBRARIES} ${QT_QTDBUS_LIBRARIES})

install(TARGETS akonadi_debugger DESTINATION ${BIN_INSTALL_DIR} )

