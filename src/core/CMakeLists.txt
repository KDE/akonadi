
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
    collectionidentificationattribute.cpp
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

ecm_generate_headers(AkonadiCore_base_HEADERS
    HEADER_NAMES
    AbstractDifferencesReporter
    AgentInstance
    AgentManager
    AgentType
    Attribute
    AttributeEntity
    AttributeFactory
    CachePolicy
    ChangeRecorder
    Collection
    CollectionFetchScope
    CollectionQuotaAttribute
    CollectionStatistics
    CollectionUtils
    CollectionIdentificationAttribute
    Control
    DBusConnectionPool
    DifferencesAlgorithmInterface
    Entity
    EntityAnnotationsAttribute
    EntityDeletedAttribute
    EntityDisplayAttribute
    EntityHiddenAttribute
    Exception
    GidExtractorInterface
    IndexPolicyAttribute
    Item
    ItemFetchScope
    ItemMonitor
    ItemSerializerPlugin
    ItemSync
    KDSignalBlocker
    MimeTypeChecker
    Monitor
    PartFetcher
    PersistentSearchAttribute
    SearchQuery
    ServerManager
    Session
    SpecialCollections
    SpecialCollectionAttribute
    Supertrait
    Tag
    TagAttribute
    TagFetchScope
    TrashSettings
    CollectionPathResolver
    REQUIRED_HEADERS AkonadiCore_base_HEADERS
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

ecm_generate_headers(AkonadiCore_models_HEADERS
    HEADER_NAMES
    AgentFilterProxyModel
    AgentInstanceModel
    AgentTypeModel
    CollectionFilterProxyModel
    CollectionModel
    CollectionStatisticsModel
    EntityMimeTypeFilterModel
    EntityOrderProxyModel
    EntityRightsFilterModel
    EntityTreeModel
    FavoriteCollectionsModel
    ItemModel
    RecursiveCollectionFilterProxyModel
    SelectionProxyModel
    StatisticsProxyModel
    TagModel
    TrashFilterProxyModel
    REQUIRED_HEADERS AkonadiCore_models_HEADERS
    RELATIVE models
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

ecm_generate_headers(AkonadiCore_jobs_HEADERS
    HEADER_NAMES
    AgentInstanceCreateJob
    CollectionAttributesSynchronizationJob
    CollectionCopyJob
    CollectionCreateJob
    CollectionDeleteJob
    CollectionFetchJob
    CollectionModifyJob
    CollectionMoveJob
    CollectionStatisticsJob
    ItemCopyJob
    ItemCreateJob
    ItemDeleteJob
    ItemFetchJob
    ItemModifyJob
    ItemMoveJob
    ItemSearchJob
    Job
    LinkJob
    RecursiveItemFetchJob
    ResourceSynchronizationJob
    SearchCreateJob
    SpecialCollectionsDiscoveryJob
    SpecialCollectionsRequestJob
    TagCreateJob
    TagDeleteJob
    TagFetchJob
    TagModifyJob
    TransactionJobs
    TransactionSequence
    TrashJob
    TrashRestoreJob
    UnlinkJob
    REQUIRED_HEADERS AkonadiCore_jobs_HEADERS
    RELATIVE jobs
)

set(akonadicore_dbus_xml ${AKONADI_DBUS_INTERFACES_DIR}/org.freedesktop.Akonadi.NotificationManager.xml)
set_source_files_properties(${akonadicore_dbus_xml} PROPERTIES INCLUDE "${AKONADI_INCLUDE_DIR}/akonadi/private/notificationmessagev3_p.h" )
qt5_add_dbus_interface(akonadicore_dbus_SRCS ${akonadicore_dbus_xml} notificationmanagerinterface)

set(akonadicore_dbus_xml ${AKONADI_DBUS_INTERFACES_DIR}/org.freedesktop.Akonadi.NotificationSource.xml)
set_source_files_properties(${akonadicore_dbus_xml} PROPERTIES INCLUDE "${AKONADI_INCLUDE_DIR}/akonadi/private/notificationmessagev3_p.h" )
qt5_add_dbus_interface(akonadicore_dbus_SRCS ${akonadicore_dbus_xml} notificationsourceinterface)

qt5_add_dbus_interfaces(akonadicore_dbus_SRCS ${AKONADI_DBUS_INTERFACES_DIR}/org.freedesktop.Akonadi.AgentManager.xml)
qt5_add_dbus_interfaces(akonadicore_dbus_SRCS ${AKONADI_DBUS_INTERFACES_DIR}/org.freedesktop.Akonadi.Tracer.xml)
qt5_add_dbus_interfaces(akonadicore_dbus_SRCS ${AKONADI_DBUS_INTERFACES_DIR}/org.freedesktop.Akonadi.Agent.Control.xml)

set(akonadicore_SRCS
    ${akonadicore_base_SRCS}
    ${akonadicore_jobs_SRCS}
    ${akonadicore_models_SRCS}
    ${akonadicore_dbus_SRCS}
)

add_library(KF5AkonadiCore ${akonadicore_SRCS})

generate_export_header(KF5AkonadiCore BASE_NAME akonadicore)

add_library(KF5::AkonadiCore ALIAS KF5AkonadiCore)
target_include_directories(KF5AkonadiCore INTERFACE "$<INSTALL_INTERFACE:${KF5_INCLUDE_INSTALL_DIR}/AkonadiCore;${Boost_INCLUDE_DIR};${AKONADI_INCLUDE_DIR};${AKONADI_INCLUDE_DIR}/akonadi/private>")
target_include_directories(KF5AkonadiCore PUBLIC "$<BUILD_INTERFACE:${Akonadi_SOURCE_DIR}/src/core;${Akonadi_BINARY_DIR}/src/core>")
target_include_directories(KF5AkonadiCore PUBLIC "$<BUILD_INTERFACE:${Akonadi_BINARY_DIR};${Akonadi_SOURCE_DIR}/src/core/jobs;${Akonadi_SOURCE_DIR}/src/core/models>")
target_include_directories(KF5AkonadiCore PUBLIC "$<BUILD_INTERFACE:${AKONADI_INCLUDE_DIR}>")

kde_target_enable_exceptions(KF5AkonadiCore PUBLIC)

target_link_libraries(KF5AkonadiCore
PUBLIC
    KF5::ItemModels
    KF5::KIOCore
    KF5::KDELibs4Support
    ${AKONADI_COMMON_LIBRARIES}
)

set_target_properties(KF5AkonadiCore PROPERTIES
    VERSION ${AKONADILIBRARIES_VERSION_STRING}
    SOVERSION ${AKONADILIBRARIES_SOVERSION}
    EXPORT_NAME AkonadiCore
)

ecm_generate_pri_file(BASE_NAME AkonadiCore
    LIB_NAME KF5AkonadiCore
    DEPS "ItemModels KIOCore KDELibs4Support" FILENAME_VAR PRI_FILENAME
)

install(TARGETS
    KF5AkonadiCore
    EXPORT KF5AkonadiTargets ${KF5_INSTALL_TARGETS_DEFAULT_ARGS}
)

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/akonadicore_export.h
    ${AkonadiCore_base_HEADERS}
    ${AkonadiCore_models_HEADERS}
    ${AkonadiCore_jobs_HEADERS}
    ${AkonadiCore_HEADERS}
    itempayloadinternals_p.h
    DESTINATION ${KF5_INCLUDE_INSTALL_DIR}/AkonadiCore COMPONENT Devel
)

install(FILES
    ${PRI_FILENAME}
    DESTINATION ${ECM_MKSPECS_INSTALL_DIR}
)
