
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

# libakonadiprotocol
set( akonadiproto_srcs
  imapparser.cpp
  imapset.cpp
  notificationmessage.cpp
)

kde4_add_library( akonadiprotocol SHARED ${akonadiproto_srcs} )

target_link_libraries( akonadiprotocol ${QT_QTCORE_LIBRARY} ${QT_QTDBUS_LIBRARY})
set_target_properties( akonadiprotocol PROPERTIES VERSION ${GENERIC_LIB_VERSION} SOVERSION ${GENERIC_LIB_SOVERSION} )
install(TARGETS akonadiprotocol  DESTINATION ${LIB_INSTALL_DIR})


# libakonadi

set( akonadi_LIB_SRC
	agentinstancemodel.cpp
	agentbase.cpp
	agentinstancecreatejob.cpp
	agentmanager.cpp
	agenttypemodel.cpp
	agentfilterproxymodel.cpp
	cachepolicy.cpp
	cachepolicypage.cpp
	changerecorder.cpp
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
	collectionpropertiesdialog.cpp
	collectionpropertiespage.cpp
	collectiongeneralpropertiespage.cpp
	collectionrightsattribute.cpp
	collectionselectjob.cpp
	collectionstatus.cpp
	collectionstatusjob.cpp
	collectionsync.cpp
	collectionview.cpp
	control.cpp
	datareference.cpp
	expungejob.cpp
	flatcollectionproxymodel.cpp
	item.cpp
	itemappendjob.cpp
	itembrowser.cpp
	itemcopyjob.cpp
	itemdeletejob.cpp
	itemdetailsview.cpp
	itemfetchjob.cpp
	itemmodel.cpp
	itemserializer.cpp
	itemserializerplugin.cpp
	itemstorejob.cpp
	itemsync.cpp
	itemview.cpp
	job.cpp
	kcrash.cpp
	messagecollectionmodel.cpp
	monitor.cpp
	monitor_p.cpp
	pastehelper.cpp
	profilemanager.cpp
	profilemodel.cpp
	protocolhelper.cpp
	resourcebase.cpp
	resourcescheduler.cpp
	searchcreatejob.cpp
	session.cpp
	standardactionmanager.cpp
	subscriptionjob.cpp
	subscriptionmodel.cpp
	subscriptiondialog.cpp
	subscriptionchangeproxymodel.cpp
	transactionjobs.cpp
	xdgbasedirs.cpp
)

qt4_generate_dbus_interface(resource.h org.kde.Akonadi.Resource.xml)
qt4_generate_dbus_interface(agent.h org.kde.Akonadi.Agent.xml)
qt4_add_dbus_interface2(akonadi_LIB_SRC ${CMAKE_CURRENT_SOURCE_DIR}/../server/interfaces/org.kde.Akonadi.NotificationManager.xml notificationmanagerinterface notificationmessage.h)
qt4_add_dbus_interfaces(akonadi_LIB_SRC
${CMAKE_CURRENT_SOURCE_DIR}/../server/interfaces/org.kde.Akonadi.AgentManager.xml)
qt4_add_dbus_interfaces(akonadi_LIB_SRC ${CMAKE_CURRENT_SOURCE_DIR}/../server/interfaces/org.kde.Akonadi.ProfileManager.xml)
qt4_add_dbus_interfaces(akonadi_LIB_SRC ${CMAKE_CURRENT_SOURCE_DIR}/../server/interfaces/org.kde.Akonadi.Tracer.xml)
qt4_add_dbus_adaptor( akonadi_LIB_SRC ${CMAKE_CURRENT_BINARY_DIR}/org.kde.Akonadi.Resource.xml resourcebase.h Akonadi::ResourceBase )
qt4_add_dbus_adaptor( akonadi_LIB_SRC ${CMAKE_CURRENT_BINARY_DIR}/org.kde.Akonadi.Agent.xml agentbase.h Akonadi::AgentBase )
kde4_add_ui_files( akonadi_LIB_SRC
  cachepolicypage.ui
  collectiongeneralpropertiespage.ui
  subscriptiondialog.ui
)

kde4_add_library( akonadicore SHARED ${akonadi_LIB_SRC} )

target_link_libraries( akonadicore ${QT_QTNETWORK_LIBRARY} ${QT_QTDBUS_LIBRARY} ${KDE4_KDEUI_LIBS} kdepim akonadiprotocol)
set_target_properties( akonadicore PROPERTIES VERSION ${GENERIC_LIB_VERSION} SOVERSION ${GENERIC_LIB_SOVERSION} )
install(TARGETS akonadicore  DESTINATION ${LIB_INSTALL_DIR})

########### install files ###############

install( FILES
	akonadi_export.h
	agentbase.h
	agentinstancecreatejob.h
	agentmanager.h
	agentfilterproxymodel.h
	agentinstancemodel.h
	agenttypemodel.h
	cachepolicy.h
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
	collectionpropertiesdialog.h
	collectionpropertiespage.h
	collectionselectjob.h
	collectionstatus.h
	collectionstatusjob.h
	collectionview.h
	control.h
	datareference.h
	expungejob.h
	item.h
	itemappendjob.h
	itembrowser.h
	itemcopyjob.h
	itemdeletejob.h
	itemdetailsview.h
	itemfetchjob.h
	itempayloadinternals_p.h
	itemserializerplugin.h
	itemstorejob.h
        itemsync.h
	itemmodel.h
	itemview.h
	job.h
	libakonadi_export.h
	messagecollectionmodel.h
	monitor.h
	profilemanager.h
	profilemodel.h
	resource.h
	resourcebase.h
	session.h
	standardactionmanager.h
	transactionjobs.h
	DESTINATION ${INCLUDE_INSTALL_DIR}/libakonadi
)
