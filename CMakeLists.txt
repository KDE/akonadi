
add_subdirectory( tests )

include_directories(
	${CMAKE_SOURCE_DIR}/
	${CMAKE_SOURCE_DIR}/libakonadi
	${CMAKE_SOURCE_DIR}/libkmime
	${QDBUS_INCLUDE_DIRS}
)


########### next target ###############

set( akonadi_LIB_SRC
	datarequest.cpp
	job.cpp
	query.cpp
	collection.cpp
	collectioncreatejob.cpp
	collectionlistjob.cpp
	collectionmodel.cpp
	collectionfetchjob.cpp
	collectionrenamejob.cpp
	collectionselectjob.cpp
	collectionview.cpp
	imapparser.cpp
	item.cpp
	itemappendjob.cpp
	messagecollection.cpp
	messagecollectionmodel.cpp
	message.cpp
	messagefetchjob.cpp
	messagemodel.cpp
	messagequery.cpp
	monitor.cpp
)

qdbus_generate_interface(${CMAKE_SOURCE_DIR}/akonadiserver/src/notification/notificationmanager.h)
qdbus_add_interfaces(akonadi_LIB_SRC ${CMAKE_CURRENT_BINARY_DIR}/notificationmanager.xml)
kde4_automoc( ${akonadi_LIB_SRC} )
kde4_add_library( akonadi SHARED ${akonadi_LIB_SRC} )
target_link_libraries( akonadi ${QT_QTNETWORK_LIBRARY} ${QDBUS_LIBRARIES} ${KDE4_KDECORE_LIBS} kmime )
set_target_properties( akonadi PROPERTIES VERSION 1.0.0 SOVERSION 1 )
install_targets( ${LIB_INSTALL_DIR} akonadi )

########### install files ###############

install_files( /include/libakonadi FILES
	datarequest.h
	job.h
	query.h
	collection.h
	collectioncreatejob.h
	collectionlistjob.h
	collectionmodel.h
	collectionfetchjob.h
	collectionrenamejob.h
	collectionselectjob.h
	collectionview.h
	imapparser.h
	item.h
	itemappendjob.h
	messagecollection.h
	messagecollectionmodel.h
	message.h
	messagefetchjob.h
	messagemodel.h
	messagequery.h
	monitor.h
)


