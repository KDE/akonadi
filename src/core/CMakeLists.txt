
set(akonadicore_base_SRCS
    agentinstance.cpp
    agentmanager.cpp
    agenttype.cpp
    asyncselectionhandler.cpp
    attribute.cpp
    attributefactory.cpp
    cachepolicy.cpp
    changemediator_p.cpp
    changenotificationdependenciesfactory.cpp
    changerecorder.cpp
    changerecorder_p.cpp
    connection.cpp
    collection.cpp
    collectioncolorattribute.cpp
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
    itemchangelog.cpp
    itemfetchscope.cpp
    itemmonitor.cpp
    itemserializer.cpp
    itemserializerplugin.cpp
    itemsync.cpp
    kdsignalblocker.cpp
    mimetypechecker.cpp
    monitor.cpp
    monitor_p.cpp
    newmailnotifierattribute.cpp
    notificationsource_p.cpp
    partfetcher.cpp
    pastehelper.cpp
    persistentsearchattribute.cpp
    pluginloader.cpp
    pop3resourceattribute.cpp
    protocolhelper.cpp
    relation.cpp
    relationsync.cpp
    searchquery.cpp
    servermanager.cpp
    session.cpp
    sessionthread.cpp
    specialcollectionattribute.cpp
    specialcollections.cpp
    tag.cpp
    tagattribute.cpp
    tagfetchscope.cpp
    tagsync.cpp
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
    AttributeFactory
    CachePolicy
    ChangeRecorder
    Collection
    CollectionColorAttribute
    CollectionFetchScope
    CollectionQuotaAttribute
    CollectionStatistics
    CollectionUtils
    CollectionIdentificationAttribute
    Control
    DifferencesAlgorithmInterface
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
    NewMailNotifierAttribute
    Monitor
    PartFetcher
    PersistentSearchAttribute
    Pop3ResourceAttribute
    Relation
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
    VectorHelper
    REQUIRED_HEADERS AkonadiCore_base_HEADERS
)

set(akonadicore_models_SRCS
    models/agentfilterproxymodel.cpp
    models/agentinstancemodel.cpp
    models/agenttypemodel.cpp
    models/collectionfilterproxymodel.cpp
    models/collectionmodel.cpp
    models/collectionmodel_p.cpp
    models/entitymimetypefiltermodel.cpp
    models/entityorderproxymodel.cpp
    models/entityrightsfiltermodel.cpp
    models/entitytreemodel.cpp
    models/entitytreemodel_p.cpp
    models/favoritecollectionsmodel.cpp
    models/itemmodel.cpp
    models/quotacolorproxymodel.cpp
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
    ItemModel
    QuotaColorProxyModel
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

# This is a workaround for conflict between our "Exception" fancy header and
# C++ stdlib's "exception" header which occurs in case-insensitive systems.
# For that reason we generate std_exception.h file, which contains an absolute
# path to the stdlib's exception header file, which resolves the ambiguity
# when including <exception> from within Akonadi.
include(FindStdlibInclude)
findStdlibInclude("exception" std_exception_file)
if (NOT "${std_exception_file}" STREQUAL "")
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/std_exception.h.in
                   ${CMAKE_CURRENT_BINARY_DIR}/std_exception.h
    )
else()
    message(FATAL_ERROR "stdlib <exception> include absolute path not found")
endif()

set(akonadicore_dbus_xml ${Akonadi_SOURCE_DIR}/src/interfaces/org.freedesktop.Akonadi.NotificationManager.xml)
qt5_add_dbus_interface(akonadicore_dbus_SRCS ${akonadicore_dbus_xml} notificationmanagerinterface)

set(akonadicore_dbus_xml ${Akonadi_SOURCE_DIR}/src/interfaces/org.freedesktop.Akonadi.NotificationSource.xml)
set_source_files_properties(${akonadicore_dbus_xml} PROPERTIES INCLUDE "${Akonadi_SOURCE_DIR}/src/private/protocol_p.h" )
qt5_add_dbus_interface(akonadicore_dbus_SRCS ${akonadicore_dbus_xml} notificationsourceinterface)

qt5_add_dbus_interfaces(akonadicore_dbus_SRCS ${Akonadi_SOURCE_DIR}/src/interfaces/org.freedesktop.Akonadi.AgentManager.xml)
qt5_add_dbus_interfaces(akonadicore_dbus_SRCS ${Akonadi_SOURCE_DIR}/src/interfaces/org.freedesktop.Akonadi.Tracer.xml)
qt5_add_dbus_interfaces(akonadicore_dbus_SRCS ${Akonadi_SOURCE_DIR}/src/interfaces/org.freedesktop.Akonadi.Agent.Control.xml)

set(akonadicore_SRCS
    ${akonadicore_base_SRCS}
    ${akonadicore_jobs_SRCS}
    ${akonadicore_models_SRCS}
    ${akonadicore_dbus_SRCS}
)

ecm_qt_declare_logging_category(akonadicore_SRCS HEADER akonadicore_debug.h IDENTIFIER AKONADICORE_LOG CATEGORY_NAME log_akonadicore)

add_library(KF5AkonadiCore ${akonadicore_SRCS})

generate_export_header(KF5AkonadiCore BASE_NAME akonadicore)

add_library(KF5::AkonadiCore ALIAS KF5AkonadiCore)
target_include_directories(KF5AkonadiCore INTERFACE "$<INSTALL_INTERFACE:${KDE_INSTALL_INCLUDEDIR_KF5}/AkonadiCore;${Boost_INCLUDE_DIR}>")
target_include_directories(KF5AkonadiCore PUBLIC "$<BUILD_INTERFACE:${Akonadi_SOURCE_DIR}/src/core;${Akonadi_BINARY_DIR}/src/core;${Akonadi_BINARY_DIR}/src/private>")
target_include_directories(KF5AkonadiCore PUBLIC "$<BUILD_INTERFACE:${Akonadi_BINARY_DIR};${Akonadi_SOURCE_DIR}/src/core/jobs;${Akonadi_SOURCE_DIR}/src/core/models>")
target_include_directories(KF5AkonadiCore PUBLIC "$<BUILD_INTERFACE:${Boost_INCLUDE_DIR}>")

kde_target_enable_exceptions(KF5AkonadiCore PUBLIC)

target_link_libraries(KF5AkonadiCore
PUBLIC
    KF5::CoreAddons # for KJob
    KF5::ItemModels
    Qt5::Gui # for QColor
PRIVATE
    Qt5::Widgets
    Qt5::Network
    KF5::AkonadiPrivate
    KF5::DBusAddons
    KF5::I18n
    KF5::IconThemes
    KF5::ConfigCore
    KF5AkonadiPrivate
)

set_target_properties(KF5AkonadiCore PROPERTIES
    VERSION ${AKONADI_VERSION_STRING}
    SOVERSION ${AKONADI_SOVERSION}
    EXPORT_NAME AkonadiCore
)

ecm_generate_pri_file(BASE_NAME AkonadiCore
    LIB_NAME KF5AkonadiCore
    DEPS "ItemModels CoreAddons" FILENAME_VAR PRI_FILENAME
)

install(TARGETS
    KF5AkonadiCore
    EXPORT KF5AkonadiTargets ${KF5_INSTALL_TARGETS_DEFAULT_ARGS}
)

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/akonadicore_export.h
    ${CMAKE_CURRENT_BINARY_DIR}/std_exception.h
    ${AkonadiCore_base_HEADERS}
    ${AkonadiCore_models_HEADERS}
    ${AkonadiCore_jobs_HEADERS}
    ${AkonadiCore_HEADERS}
    qtest_akonadi.h
    itempayloadinternals_p.h
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
