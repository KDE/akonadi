# Turn exceptions on
kde_enable_exceptions()

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/jobs
    ${CMAKE_CURRENT_SOURCE_DIR}/models
)

set(akonadicore_base_SRCS
    agentinstance.cpp
    agentmanager.cpp
    agenttype.cpp
    asyncselectionhandler.cpp
    attribute.cpp
    attributeentity.cpp
    attributefactory.cpp
    cachepolicy.cpp
    changemediator_p.cpp
    changenotificationdependenciesfactory.cpp
    changerecorder.cpp
    changerecorder_p.cpp
    collection.cpp
    collectionfetchscope.cpp
    collectionpathresolver.cpp
    collectionquotaattribute.cpp
    collectionquotaattribute.cpp
    collectionrightsattribute.cpp
    collectionstatistics.cpp
    collectionsync.cpp
    conflicthandler.cpp
    control.cpp
    dbusconnectionpool.cpp
    entity.cpp
    entityannotationsattribute.cpp
    entitycache.cpp
    entitydeletedattribute.cpp
    entitydeletedattribute.cpp
    entitydisplayattribute.cpp
    entityhiddenattribute.cpp
    exception.cpp
    firstrun.cpp
    gidextractor.cpp
    indexpolicyattribute.cpp
    item.cpp
    itemfetchscope.cpp
    itemmonitor.cpp
    itemserializer.cpp
    itemserializerplugin.cpp
    itemsync.cpp
    kdsignalblocker.cpp
    mimetypechecker.cpp
    monitor.cpp
    monitor_p.cpp
    notificationsource.cpp
    partfetcher.cpp
    pastehelper.cpp
    persistentsearchattribute.cpp
    pluginloader.cpp
    protocolhelper.cpp
    searchquery.cpp
    servermanager.cpp
    session.cpp
    specialcollectionattribute.cpp
    specialcollections.cpp
    tag.cpp
    tagattribute.cpp
    tagfetchscope.cpp
    trashsettings.cpp
    typepluginloader.cpp
)

set(akonadicore_models_SRCS
    models/agentfilterproxymodel.cpp
    models/agentinstancemodel.cpp
    models/agenttypemodel.cpp
    models/collectionfilterproxymodel.cpp
    models/collectionmodel.cpp
    models/collectionmodel_p.cpp
    models/collectionstatisticsmodel.cpp
    models/entitymimetypefiltermodel.cpp
    models/entityorderproxymodel.cpp
    models/entityrightsfiltermodel.cpp
    models/entitytreemodel.cpp
    models/entitytreemodel_p.cpp
    models/favoritecollectionsmodel.cpp
    models/itemmodel.cpp
    models/recursivecollectionfilterproxymodel.cpp
    models/selectionproxymodel.cpp
    models/statisticsproxymodel.cpp
    models/subscriptionmodel.cpp
    models/tagmodel.cpp
    models/tagmodel_p.cpp
    models/trashfilterproxymodel.cpp
)

set(akonadicore_jobs_SRCS
    jobs/agentinstancecreatejob.cpp
    jobs/collectionattributessynchronizationjob.cpp
    jobs/collectioncopyjob.cpp
    jobs/collectioncreatejob.cpp
    jobs/collectiondeletejob.cpp
    jobs/collectionfetchjob.cpp
    jobs/collectionmodifyjob.cpp
    jobs/collectionmovejob.cpp
    jobs/collectionselectjob.cpp
    jobs/collectionstatisticsjob.cpp
    jobs/invalidatecachejob.cpp
    jobs/itemcopyjob.cpp
    jobs/itemcreatejob.cpp
    jobs/itemdeletejob.cpp
    jobs/itemfetchjob.cpp
    jobs/itemmodifyjob.cpp
    jobs/itemmovejob.cpp
    jobs/itemsearchjob.cpp
    jobs/job.cpp
    jobs/kjobprivatebase.cpp
    jobs/linkjob.cpp
    jobs/recursiveitemfetchjob.cpp
    jobs/resourceselectjob.cpp
    jobs/resourcesynchronizationjob.cpp
    jobs/searchcreatejob.cpp
    jobs/searchresultjob.cpp
    jobs/specialcollectionsdiscoveryjob.cpp
    jobs/specialcollectionshelperjobs.cpp
    jobs/specialcollectionsrequestjob.cpp
    jobs/subscriptionjob.cpp
    jobs/tagcreatejob.cpp
    jobs/tagdeletejob.cpp
    jobs/tagfetchjob.cpp
    jobs/tagmodifyjob.cpp
    jobs/transactionjobs.cpp
    jobs/transactionsequence.cpp
    jobs/trashjob.cpp
    jobs/trashrestorejob.cpp
    jobs/unlinkjob.cpp

)

# FIXME KF5: Remove once we merge Akonadi repository in
# This is so that we find
include_directories("${CMAKE_INSTALL_PREFIX}/include")

set(akonadicore_dbus_xml ${AKONADI_DBUS_INTERFACES_DIR}/org.freedesktop.Akonadi.NotificationManager.xml)
set_source_files_properties(${akonadicore_dbus_xml} PROPERTIES INCLUDE "akonadi/private/notificationmessagev3_p.h" )
qt5_add_dbus_interface(akonadicore_dbus_SRCS ${akonadicore_dbus_xml} notificationmanagerinterface )

set(akonadicore_dbus_xml ${AKONADI_DBUS_INTERFACES_DIR}/org.freedesktop.Akonadi.NotificationSource.xml)
set_source_files_properties(${akonadicore_dbus_xml} PROPERTIES INCLUDE "akonadi/private/notificationmessagev3_p.h" )
qt5_add_dbus_interface(akonadicore_dbus_SRCS ${akonadicore_dbus_xml} notificationsourceinterface )


qt5_add_dbus_interfaces(akonadicore_dbus_SRCS ${AKONADI_DBUS_INTERFACES_DIR}/org.freedesktop.Akonadi.AgentManager.xml )
qt5_add_dbus_interfaces(akonadicore_dbus_SRCS ${AKONADI_DBUS_INTERFACES_DIR}/org.freedesktop.Akonadi.Tracer.xml )
qt5_add_dbus_interfaces(akonadicore_dbus_SRCS ${AKONADI_DBUS_INTERFACES_DIR}/org.freedesktop.Akonadi.Agent.Control.xml )

set(akonadicore_SRCS
    ${akonadicore_base_SRCS}
    ${akonadicore_jobs_SRCS}
    ${akonadicore_models_SRCS}
    ${akonadicore_dbus_SRCS}
)

add_library(KF5AkonadiCore ${akonadicore_SRCS})

# qt4_add_dbus_adaptor( akonadikde_LIB_SRC ${AKONADI_DBUS_INTERFACES_DIR}/org.freedesktop.Akonadi.Resource.xml resourcebase.h Akonadi::ResourceBase resourceadaptor Akonadi__ResourceAdaptor )
# qt4_add_dbus_adaptor( akonadikde_LIB_SRC ${AKONADI_DBUS_INTERFACES_DIR}/org.freedesktop.Akonadi.Preprocessor.xml preprocessorbase_p.h Akonadi::PreprocessorBasePrivate preprocessoradaptor Akonadi__PreprocessorAdaptor )
# qt4_add_dbus_adaptor( akonadikde_LIB_SRC ${AKONADI_DBUS_INTERFACES_DIR}/org.freedesktop.Akonadi.Agent.Status.xml agentbase.h Akonadi::AgentBase statusadaptor Akonadi__StatusAdaptor )
# qt4_add_dbus_adaptor( akonadikde_LIB_SRC ${AKONADI_DBUS_INTERFACES_DIR}/org.freedesktop.Akonadi.Agent.Control.xml agentbase.h Akonadi::AgentBase controladaptor Akonadi__ControlAdaptor)
# qt4_add_dbus_adaptor( akonadikde_LIB_SRC interfaces/org.freedesktop.Akonadi.Resource.Transport.xml transportresourcebase_p.h Akonadi::TransportResourceBasePrivate transportadaptor Akonadi__TransportAdaptor )
# qt4_add_dbus_adaptor( akonadikde_LIB_SRC interfaces/org.freedesktop.Akonadi.Agent.Search.xml agentsearchinterface_p.h Akonadi::AgentSearchInterfacePrivate searchadaptor Akonadi__SearchAdaptor )
#


generate_export_header(KF5AkonadiCore BASE_NAME akonadicore)

add_library(KF5::AkonadiCore ALIAS KF5AkonadiCore)

target_include_directories(KF5AkonadiCore INTERFACE "$<INSTALL_INTERFACE:${INCLUDE_INSTALL_DIR}/akonadi;${Boost_INCLUDE_DIR}>")
target_include_directories(KF5AkonadiCore PUBLIC "$<BUILD_INTERFACE:${AkonadiCore_SOURCE_DIR}/src;${AkonadiCore_BINARY_DIR}/src>")
target_include_directories(KF5AkonadiCore PUBLIC "$<BUILD_INTERFACE:${AkonadiCore_SOURCE_DIR}/src/core;${AkonadiCore_BINARY_DIR}/src/core>")

target_link_libraries(KF5AkonadiCore
    KF5::ItemModels
    KF5::KIOCore
    KF5::KDE4Support
    ${CMAKE_INSTALL_PREFIX}/${LIB_INSTALL_DIR}/libakonadiprotocolinternals.so
)

set_target_properties(KF5AkonadiCore PROPERTIES
    VERSION ${AKONADI_VERSION_STRING}
    SOVERSION ${AKONADI_SOVERSION}
    EXPORT_NAME AkonadiCore
)

# ecm_generate_headers(KSyndication_CamelCase_HEADERS
#   HEADER_NAMES
#   AbstractParser
#   REQUIRED_HEADERS KSyndication_HEADERS
#   PREFIX Syndication
# )


install(TARGETS KF5AkonadiCore EXPORT KF5AkonadiTargets ${INSTALL_TARGETS_DEFAULT_ARGS})

install(FILES
    #${KSyndication_CamelCase_HEADERS}
    DESTINATION ${INCLUDE_INSTALL_DIR}/Akonadi
    COMPONENT Devel
)

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/akonadicore_export.h
    ${AkonadiCore_HEADERS}
    DESTINATION ${INCLUDE_INSTALL_DIR}/akonadi COMPONENT Devel)

ecm_generate_pri_file(BASE_NAME AkonadiCore LIB_NAME KF5AkonadiCore DEPS "ItemModels KIOCore KDE4Support" FILENAME_VAR PRI_FILENAME)
install(FILES ${PRI_FILENAME} DESTINATION ${ECM_MKSPECS_INSTALL_DIR})
