configure_file(akonaditests_export.h.in "${CMAKE_CURRENT_BINARY_DIR}/akonaditests_export.h")

set(akonadicore_base_SRCS
    agentconfigurationbase.cpp
    agentconfigurationfactorybase.cpp
    agentconfigurationmanager.cpp
    agentinstance.cpp
    agentmanager.cpp
    agenttype.cpp
    asyncselectionhandler.cpp
    attributefactory.cpp
    attributestorage.cpp
    braveheart.cpp
    cachepolicy.cpp
    changemediator_p.cpp
    changenotification.cpp
    changenotificationdependenciesfactory.cpp
    changerecorder.cpp
    changerecorder_p.cpp
    changerecorderjournal.cpp
    connection.cpp
    collection.cpp
    collectionfetchscope.cpp
    collectionpathresolver.cpp
    collectionstatistics.cpp
    collectionsync.cpp
    conflicthandler.cpp
    control.cpp
    entitycache.cpp
    exception.cpp
    firstrun.cpp
    gidextractor.cpp
    item.cpp
    itemchangelog.cpp
    itemfetchscope.cpp
    itemmonitor.cpp
    itemserializer.cpp
    itemserializerplugin.cpp
    itemsync.cpp
    mimetypechecker.cpp
    monitor.cpp
    monitor_p.cpp
    notificationsource_p.cpp
    notificationsubscriber.cpp
    partfetcher.cpp
    pastehelper.cpp
    pluginloader.cpp
    protocolhelper.cpp
    remotelog.cpp
    relation.cpp
    relationsync.cpp
    searchquery.cpp
    servermanager.cpp
    session.cpp
    sessionthread.cpp
    specialcollections.cpp
    tag.cpp
    tagfetchscope.cpp
    tagsync.cpp
    trashsettings.cpp
    typepluginloader.cpp
)

ecm_generate_headers(AkonadiCore_base_HEADERS
    HEADER_NAMES
    AbstractDifferencesReporter
    AgentConfigurationBase
    AgentConfigurationFactoryBase
    AgentInstance
    AgentManager
    AgentType
    AttributeFactory
    CachePolicy
    ChangeNotification
    ChangeRecorder
    Collection
    CollectionFetchScope
    CollectionStatistics
    CollectionUtils
    Control
    DifferencesAlgorithmInterface
    ExceptionBase
    GidExtractorInterface
    Item
    ItemFetchScope
    ItemMonitor
    ItemSerializerPlugin
    ItemSync
    MimeTypeChecker
    NotificationSubscriber
    Monitor
    PartFetcher
    Relation
    SearchQuery
    ServerManager
    Session
    SpecialCollections
    Supertrait
    Tag
    TagFetchScope
    TrashSettings
    CollectionPathResolver
    REQUIRED_HEADERS AkonadiCore_base_HEADERS
)

set(akonadicore_attributes_SRCS
    attributes/attribute.cpp
    attributes/collectioncolorattribute.cpp
    attributes/collectionquotaattribute.cpp
    attributes/collectionidentificationattribute.cpp
    attributes/collectionrightsattribute.cpp
    attributes/entityannotationsattribute.cpp
    attributes/entitydeletedattribute.cpp
    attributes/entitydeletedattribute.cpp
    attributes/entitydisplayattribute.cpp
    attributes/entityhiddenattribute.cpp
    attributes/favoritecollectionattribute.cpp
    attributes/indexpolicyattribute.cpp
    attributes/persistentsearchattribute.cpp
    attributes/specialcollectionattribute.cpp
    attributes/tagattribute.cpp
)

ecm_generate_headers(AkonadiCore_attributes_HEADERS
    HEADER_NAMES
    Attribute
    CollectionColorAttribute
    CollectionIdentificationAttribute
    CollectionQuotaAttribute
    EntityAnnotationsAttribute
    EntityDeletedAttribute
    EntityDisplayAttribute
    EntityHiddenAttribute
    FavoriteCollectionAttribute
    IndexPolicyAttribute
    PersistentSearchAttribute
    SpecialCollectionAttribute
    TagAttribute
    REQUIRED_HEADERS AkonadiCore_attributes_HEADERS
    RELATIVE attributes
)

set(akonadicore_models_SRCS
    models/agentfilterproxymodel.cpp
    models/agentinstancemodel.cpp
    models/agenttypemodel.cpp
    models/collectionfilterproxymodel.cpp
    models/entitymimetypefiltermodel.cpp
    models/entityorderproxymodel.cpp
    models/entityrightsfiltermodel.cpp
    models/entitytreemodel.cpp
    models/entitytreemodel_p.cpp
    models/favoritecollectionsmodel.cpp
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
    EntityMimeTypeFilterModel
    EntityOrderProxyModel
    EntityRightsFilterModel
    EntityTreeModel
    FavoriteCollectionsModel
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
    jobs/relationfetchjob.cpp
    jobs/relationcreatejob.cpp
    jobs/relationdeletejob.cpp
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
    RelationFetchJob
    RelationCreateJob
    RelationDeleteJob
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

set(akonadicore_dbus_xml ${Akonadi_SOURCE_DIR}/src/interfaces/org.freedesktop.Akonadi.NotificationManager.xml)
qt5_add_dbus_interface(akonadicore_dbus_SRCS ${akonadicore_dbus_xml} notificationmanagerinterface)

set(akonadicore_dbus_xml ${Akonadi_SOURCE_DIR}/src/interfaces/org.freedesktop.Akonadi.NotificationSource.xml)
set_source_files_properties(${akonadicore_dbus_xml} PROPERTIES INCLUDE "${Akonadi_SOURCE_DIR}/src/private/protocol_p.h" )
qt5_add_dbus_interface(akonadicore_dbus_SRCS ${akonadicore_dbus_xml} notificationsourceinterface)

qt5_add_dbus_interfaces(akonadicore_dbus_SRCS ${Akonadi_SOURCE_DIR}/src/interfaces/org.freedesktop.Akonadi.AgentManager.xml)
qt5_add_dbus_interfaces(akonadicore_dbus_SRCS ${Akonadi_SOURCE_DIR}/src/interfaces/org.freedesktop.Akonadi.Tracer.xml)
qt5_add_dbus_interfaces(akonadicore_dbus_SRCS ${Akonadi_SOURCE_DIR}/src/interfaces/org.freedesktop.Akonadi.Agent.Control.xml)
qt5_add_dbus_interfaces(akonadicore_dbus_SRCS
    ${Akonadi_SOURCE_DIR}/src/interfaces/org.freedesktop.Akonadi.Resource.xml)

set(akonadicore_SRCS
    ${akonadicore_base_SRCS}
    ${akonadicore_attributes_SRCS}
    ${akonadicore_jobs_SRCS}
    ${akonadicore_models_SRCS}
    ${akonadicore_dbus_SRCS}
)

ecm_qt_declare_logging_category(akonadicore_SRCS HEADER akonadicore_debug.h IDENTIFIER AKONADICORE_LOG CATEGORY_NAME org.kde.pim.akonadicore
        DESCRIPTION "akonadi (Akonadi Core Library)"
        OLD_CATEGORY_NAMES log_akonadicore
        EXPORT AKONADI
    )


add_library(KF5AkonadiCore ${akonadicore_SRCS})

generate_export_header(KF5AkonadiCore BASE_NAME akonadicore)

add_library(KF5::AkonadiCore ALIAS KF5AkonadiCore)
target_include_directories(KF5AkonadiCore INTERFACE "$<INSTALL_INTERFACE:${KDE_INSTALL_INCLUDEDIR_KF5}/AkonadiCore>")
target_include_directories(KF5AkonadiCore PUBLIC "$<BUILD_INTERFACE:${Akonadi_SOURCE_DIR}/src/core;${Akonadi_BINARY_DIR}/src/core;${Akonadi_BINARY_DIR}/src/private>")
target_include_directories(KF5AkonadiCore PUBLIC "$<BUILD_INTERFACE:${Akonadi_BINARY_DIR};${Akonadi_SOURCE_DIR}/src/core/jobs;${Akonadi_SOURCE_DIR}/src/core/models;${Akonadi_SOURCE_DIR}/src/core/attributes>")

kde_target_enable_exceptions(KF5AkonadiCore PUBLIC)

target_link_libraries(KF5AkonadiCore
PUBLIC
    KF5::CoreAddons # for KJob
    KF5::ItemModels
    Qt5::Gui # for QColor
    Boost::boost
PRIVATE
    Qt5::Network
    Qt5::Widgets
    KF5::AkonadiPrivate
    KF5::I18n
    KF5::IconThemes
    KF5::ConfigCore
    KF5AkonadiPrivate
    akonadi_shared
)

set_target_properties(KF5AkonadiCore PROPERTIES
    VERSION ${AKONADI_VERSION_STRING}
    SOVERSION ${AKONADI_SOVERSION}
    EXPORT_NAME AkonadiCore
)

ecm_generate_pri_file(BASE_NAME AkonadiCore
    LIB_NAME KF5AkonadiCore
    DEPS "KItemModels KCoreAddons" FILENAME_VAR PRI_FILENAME
)

install(TARGETS
    KF5AkonadiCore
    EXPORT KF5AkonadiTargets ${KF5_INSTALL_TARGETS_DEFAULT_ARGS}
)

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/akonadicore_export.h
    ${AkonadiCore_base_HEADERS}
    ${AkonadiCore_attributes_HEADERS}
    ${AkonadiCore_models_HEADERS}
    ${AkonadiCore_jobs_HEADERS}
    ${AkonadiCore_HEADERS}
    qtest_akonadi.h
    itempayloadinternals_p.h
    ${Akonadi_BINARY_DIR}/config-akonadi.h
    DESTINATION ${KDE_INSTALL_INCLUDEDIR_KF5}/AkonadiCore COMPONENT Devel
)

install(FILES
    ${PRI_FILENAME}
    DESTINATION ${ECM_MKSPECS_INSTALL_DIR}
)

install( FILES
    kcfg2dbus.xsl
    DESTINATION ${KDE_INSTALL_DATADIR_KF5}/akonadi
)
