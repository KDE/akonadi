
include_directories(
	${CMAKE_SOURCE_DIR}/
	${QDBUS_INCLUDE_DIRS}
)


########### next target ###############

set( nm_SRC notificationmanager.cpp main.cpp)

qdbus_generate_interface( notificationmanager.h )
qdbus_add_adaptors( nm_SRC ${CMAKE_CURRENT_BINARY_DIR}/notificationmanager.xml )
kde4_automoc( ${nm_SRC} )
kde4_add_executable( notificationmanager ${nm_SRC} )
target_link_libraries( notificationmanager ${QT_QTCORE_LIBRARY} ${QDBUS_LIBRARIES})

#install_files( ${somewhere???} FILES  notificationmanager.xml)
