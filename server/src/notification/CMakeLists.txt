
include_directories(
	${CMAKE_SOURCE_DIR}/
	${QDBUS_INCLUDE_DIRS}
)


########### next target ###############

set( nm_SRC notificationmanager.cpp main.cpp)

qt4_add_dbus_adaptors( nm_SRC org.kde.Akonadi.NotificationManager.xml )
kde4_automoc( ${nm_SRC} )
kde4_add_executable( notificationmanager ${nm_SRC} )
target_link_libraries( notificationmanager ${QT_QTCORE_LIBRARY} ${QT_QTDBUS_LIBRARIES})

#install_files( ${somewhere???} FILES  org.kde.Akonadi.NotificationManager.xml)
