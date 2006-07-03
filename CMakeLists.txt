
add_subdirectory( tests )

include_directories(
	${CMAKE_SOURCE_DIR}/
	${CMAKE_SOURCE_DIR}/libakonadi
	${CMAKE_SOURCE_DIR}/libkmime
	${QT_QTDBUS_INCLUDE_DIR}
)


########### next target ###############

set( akonadi_LIB_SRC
	datarequest.cpp
	query.cpp
	collection.cpp
	collectionattribute.cpp
	collectioncreatejob.cpp
	collectionlistjob.cpp
	collectionmodel.cpp
	collectionrenamejob.cpp
	collectionselectjob.cpp
	collectionstatusjob.cpp
	collectionview.cpp
	imapparser.cpp
	item.cpp
	itemappendjob.cpp
	job.cpp
	jobqueue.cpp
	messagecollectionattribute.cpp
	messagecollectionmodel.cpp
	message.cpp
	messagefetchjob.cpp
	messagemodel.cpp
	messagequery.cpp
	monitor.cpp
)

qt4_add_dbus_interfaces(akonadi_LIB_SRC ${CMAKE_SOURCE_DIR}/akonadiserver/src/notification/org.kde.Akonadi.NotificationManager.xml)
kde4_automoc( ${akonadi_LIB_SRC} )
kde4_add_library( akonadi SHARED ${akonadi_LIB_SRC} )
target_link_libraries( akonadi ${QT_QTNETWORK_LIBRARY} ${QT_QTDBUS_LIBRARIES} ${KDE4_KDECORE_LIBS} kmime )
set_target_properties( akonadi PROPERTIES VERSION 1.0.0 SOVERSION 1 )
install_targets( ${LIB_INSTALL_DIR} akonadi )

########### install files ###############

install_files( /include/libakonadi FILES
	datarequest.h
	query.h
	collection.h
	collectionattribute.h
	collectioncreatejob.h
	collectionlistjob.h
	collectionmodel.h
	collectionrenamejob.h
	collectionselectjob.h
	collectionstatusjob.h
	collectionview.h
	imapparser.h
	item.h
	itemappendjob.h
	job.h
	jobqueue.h
	messagecollectionattribute.h
	messagecollectionmodel.h
	message.h
	messagefetchjob.h
	messagemodel.h
	messagequery.h
	monitor.h
)


