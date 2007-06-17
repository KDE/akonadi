
add_subdirectory( tests )
add_subdirectory( components )

include_directories(
	${CMAKE_SOURCE_DIR}/
	${CMAKE_SOURCE_DIR}/akonadi
	${CMAKE_BINARY_DIR}/akonadi
	${QT_QTDBUS_INCLUDE_DIR}
        ${Boost_INCLUDE_DIR}
)

set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DQT_NO_CAST_FROM_ASCII -DQT_NO_CAST_TO_ASCII ${KDE4_ENABLE_EXCEPTIONS}" )

########### next target ###############

set( akonadi_LIB_SRC
	agentinstancemodel.cpp
	agentmanager.cpp
	agenttypemodel.cpp
	collection.cpp
	collectionattribute.cpp
	collectionattributefactory.cpp
	collectioncreatejob.cpp
	collectiondeletejob.cpp
	collectionlistjob.cpp
	collectionmodel.cpp
	collectionfilterproxymodel.cpp
	collectionmodifyjob.cpp
	collectionpathresolver.cpp
	collectionselectjob.cpp
	collectionstatus.cpp
	collectionstatusjob.cpp
	collectionsync.cpp
	collectionview.cpp
	control.cpp
	datareference.cpp
	expungejob.cpp
	imapparser.cpp
	item.cpp
	itemappendjob.cpp
	itembrowser.cpp
	itemdeletejob.cpp
	itemfetchjob.cpp
	itemmodel.cpp
	itemserializer.cpp
	itemstorejob.cpp
	itemview.cpp
	job.cpp
	kcrash.cpp
	messagecollectionmodel.cpp
	monitor.cpp
	monitor_p.cpp
	profilemanager.cpp
	profilemodel.cpp
	resourcebase.cpp
	resourcescheduler.cpp
	session.cpp
	transactionjobs.cpp
)

qt4_generate_dbus_interface(../server/src/notificationmanager.h org.kde.Akonadi.NotificationManager.xml)
qt4_generate_dbus_interface(resource.h org.kde.Akonadi.Resource.xml)
qt4_add_dbus_interfaces(akonadi_LIB_SRC ${CMAKE_CURRENT_BINARY_DIR}/org.kde.Akonadi.NotificationManager.xml)
qt4_add_dbus_interfaces(akonadi_LIB_SRC ${CMAKE_SOURCE_DIR}/akonadi/server/interfaces/org.kde.Akonadi.AgentManager.xml)
qt4_add_dbus_interfaces(akonadi_LIB_SRC ${CMAKE_SOURCE_DIR}/akonadi/server/interfaces/org.kde.Akonadi.ProfileManager.xml)
qt4_add_dbus_interfaces(akonadi_LIB_SRC ${CMAKE_SOURCE_DIR}/akonadi/server/interfaces/org.kde.Akonadi.Tracer.xml)
qt4_add_dbus_adaptor( akonadi_LIB_SRC ${CMAKE_CURRENT_BINARY_DIR}/org.kde.Akonadi.Resource.xml resourcebase.h Akonadi::ResourceBase )

kde4_automoc(${akonadi_LIB_SRC})
kde4_add_library( akonadi SHARED ${akonadi_LIB_SRC} )

target_link_libraries( akonadi ${QT_QTNETWORK_LIBRARY} ${QT_QTDBUS_LIBRARIES} ${KDE4_KDEUI_LIBS} kdepim)
set_target_properties( akonadi PROPERTIES VERSION ${GENERIC_LIB_VERSION} SOVERSION ${GENERIC_LIB_SOVERSION} )
install(TARGETS akonadi  DESTINATION ${LIB_INSTALL_DIR})

########### install files ###############

install( FILES
	agentmanager.h
	collection.h
	collectionattribute.h
	collectionattributefactory.h
	collectioncreatejob.h
	collectiondeletejob.h
	collectionlistjob.h
	collectionmodel.h
	collectionfilterproxymodel.h
	collectionmodifyjob.h
	collectionpathresolver.h
	collectionselectjob.h
	collectionstatus.h
	collectionstatusjob.h
	collectionview.h
	control.h
	datareference.h
	expungejob.h
	imapparser.h
	item.h
	itemappendjob.h
	itembrowser.h
	itemdeletejob.h
	itemfetchjob.h
	itempayloadinternals_p.h
	itemserializer.h
	itemstorejob.h
	itemview.h
	job.h
	libakonadi_export.h
	messagecollectionmodel.h
	monitor.h
	profilemanager.h
	resource.h
	resourcebase.h
	session.h
	transactionjobs.h
	DESTINATION ${INCLUDE_INSTALL_DIR}/libakonadi
)
