
include_directories(
	${CMAKE_SOURCE_DIR}/
	${QT_QTDBUS_INCLUDE_DIR}
)


########### next target ###############

set( nm_SRC notificationmanager.cpp main.cpp)

qt4_add_dbus_adaptors( nm_SRC org.kde.Akonadi.NotificationManager.xml )

kde4_automoc( ${nm_SRC} )
kde4_add_executable( akonadi_notificationmanager ${nm_SRC} )
target_link_libraries( akonadi_notificationmanager ${QT_QTCORE_LIBRARY} ${QT_QTDBUS_LIBRARIES} ${QT_QTGUI_LIBRARIES})

#install_files( ${somewhere???} FILES  org.kde.Akonadi.NotificationManager.xml)

install_targets(/bin akonadi_notificationmanager)
