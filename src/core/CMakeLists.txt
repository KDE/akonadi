configure_file(akonaditests_export.h.in "${CMAKE_CURRENT_BINARY_DIR}/akonaditests_export.h")

# base install include dir
set(AkonadiCore_INSTALL_INCLUDEDIR "${KDE_INSTALL_INCLUDEDIR}/KPim6/AkonadiCore")

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
    config.cpp
    connection.cpp
    collection.cpp
    collectionfetchscope.cpp
    collectionpathresolver.cpp
    collectionstatistics.cpp
    collectionsync.cpp
    collectionutils.cpp collectionutils.h
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
    searchquery.cpp
    servermanager.cpp
    session.cpp
    sessionthread.cpp
    specialcollections.cpp
    tag.cpp
    tagcache.cpp
    tagfetchscope.cpp
    tagsync.cpp
    trashsettings.cpp
    typepluginloader.cpp

    agentconfigurationbase.h
    agentconfigurationfactorybase.h
    agentconfigurationmanager_p.h
    agentinstance_p.h
    agentmanager.h
    agentmanager_p.h
    agenttype.h
    asyncselectionhandler_p.h
    attributefactory.h
    attributestorage_p.h
    cachepolicy.h
    changemediator_p.h
    changenotification.h
    changenotificationdependenciesfactory_p.h
    changerecorder.h
    changerecorder_p.h
    changerecorderjournal_p.h
    config_p.h
    connection_p.h
    collection.h
    collectionfetchscope.h
    collectionpathresolver.h
    collectionstatistics.h
    collectionsync_p.h
    conflicthandler_p.h
    control.h
    entitycache_p.h
    firstrun_p.h
    gidextractor_p.h
    item.h
    itemchangelog_p.h
    itemfetchscope.h
    itemmonitor.h
    itemmonitor_p.h
    itemserializer_p.h
    itemserializerplugin.h
    itemsync.h
    mimetypechecker.h
    mimetypechecker_p.h
    monitor.h
    monitor_p.h
    notificationsource_p.h
    notificationsubscriber.h
    partfetcher.h
    pastehelper_p.h
    pluginloader_p.h
    protocolhelper_p.h
    searchquery.h
    servermanager.h
    servermanager_p.h
    session.h
    session_p.h
    sessionthread_p.h
    specialcollections.h
    specialcollections_p.h
    tag.h
    tagcache.h
    tagfetchscope.h
    tagsync.h
    trashsettings.h
    typepluginloader_p.h

    accountactivitiesabstract.h
    accountactivitiesabstract.cpp

   )

ecm_generate_headers(AkonadiCore_base_CC_HEADERS
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
    SearchQuery
    ServerManager
    Session
    SpecialCollections
    Supertrait
    Tag
    TagCache
    TagFetchScope
    TrashSettings
    CollectionPathResolver
    AccountActivitiesAbstract
    REQUIRED_HEADERS AkonadiCore_base_HEADERS
    PREFIX Akonadi
)

set(akonadicore_attributes_SRCS
    attributes/attribute.cpp
    attributes/collectionannotationsattribute.cpp attributes/collectionannotationsattribute.h
    attributes/collectioncolorattribute.cpp attributes/collectioncolorattribute.h
    attributes/collectionquotaattribute.cpp attributes/collectionquotaattribute.h
    attributes/collectionidentificationattribute.cpp attributes/collectionidentificationattribute.h
    attributes/collectionrightsattribute.cpp
    attributes/entityannotationsattribute.cpp attributes/entityannotationsattribute.h
    attributes/entitydeletedattribute.cpp attributes/entitydeletedattribute.h
    attributes/entitydeletedattribute.cpp attributes/entitydeletedattribute.h
    attributes/entitydisplayattribute.cpp attributes/entitydisplayattribute.h
    attributes/entityhiddenattribute.cpp attributes/entityhiddenattribute.h
    attributes/favoritecollectionattribute.cpp attributes/favoritecollectionattribute.h
    attributes/indexpolicyattribute.cpp attributes/indexpolicyattribute.h
    attributes/persistentsearchattribute.cpp attributes/persistentsearchattribute.h
    attributes/specialcollectionattribute.cpp attributes/specialcollectionattribute.h
    attributes/tagattribute.cpp attributes/tagattribute.h
)

ecm_generate_headers(AkonadiCore_attributes_CC_HEADERS
    HEADER_NAMES
    Attribute
    CollectionAnnotationsAttribute
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
    PREFIX Akonadi
)

set(akonadicore_models_SRCS
    models/agentfilterproxymodel.cpp models/agentfilterproxymodel.h
    models/agentinstancemodel.cpp models/agentinstancemodel.h
    models/agenttypemodel.cpp models/agenttypemodel.h
    models/collectionfilterproxymodel.cpp
    models/entitymimetypefiltermodel.cpp models/entitymimetypefiltermodel.h
    models/entityorderproxymodel.cpp models/entityorderproxymodel.h
    models/entityrightsfiltermodel.cpp models/entityrightsfiltermodel.h
    models/entitytreemodel.cpp models/entitytreemodel.h
    models/entitytreemodel_p.cpp models/entitytreemodel_p.h
    models/favoritecollectionsmodel.cpp models/favoritecollectionsmodel.h
    models/recursivecollectionfilterproxymodel.cpp models/recursivecollectionfilterproxymodel.h
    models/selectionproxymodel.cpp models/selectionproxymodel.h
    models/statisticsproxymodel.cpp models/statisticsproxymodel.h
    models/subscriptionmodel.cpp
    models/tagmodel.cpp models/tagmodel.h
    models/tagmodel_p.cpp models/tagmodel_p.h
    models/trashfilterproxymodel.cpp models/trashfilterproxymodel.h
    models/agentinstancefilterproxymodel.cpp models/agentinstancefilterproxymodel.h
)

ecm_generate_headers(AkonadiCore_models_CC_HEADERS
    HEADER_NAMES
    AgentFilterProxyModel
    AgentInstanceModel
    AgentInstanceFilterProxyModel
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
    PREFIX Akonadi
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
    jobs/searchcreatejob.cpp
    jobs/searchresultjob.cpp
    jobs/specialcollectionsdiscoveryjob.cpp jobs/specialcollectionsdiscoveryjob.h
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

    jobs/agentinstancecreatejob.h
    jobs/collectionattributessynchronizationjob.h
    jobs/collectioncopyjob.h
    jobs/collectioncreatejob.h
    jobs/collectiondeletejob.h
    jobs/collectionfetchjob.h
    jobs/collectionmodifyjob.h
    jobs/collectionmovejob.h
    jobs/collectionstatisticsjob.h
    jobs/itemcopyjob.h
    jobs/itemcreatejob.h
    jobs/itemdeletejob.h
    jobs/itemfetchjob.h
    jobs/itemmodifyjob.h
    jobs/itemmovejob.h
    jobs/itemsearchjob.h
    jobs/job.h
    jobs/kjobprivatebase_p.h
    jobs/linkjob.h
    jobs/recursiveitemfetchjob.h
    jobs/resourceselectjob_p.h
    jobs/resourcesynchronizationjob.h
    jobs/searchcreatejob.h
    jobs/searchresultjob_p.h
    jobs/specialcollectionshelperjobs_p.h
    jobs/specialcollectionsrequestjob.h
    jobs/subscriptionjob_p.h
    jobs/tagcreatejob.h
    jobs/tagdeletejob.h
    jobs/tagfetchjob.h
    jobs/tagmodifyjob.h
    jobs/transactionjobs.h
    jobs/transactionsequence.h
    jobs/trashjob.h
    jobs/trashrestorejob.h
    jobs/unlinkjob.h

   )

ecm_generate_headers(AkonadiCore_jobs_CC_HEADERS
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
    PREFIX Akonadi
)

set(akonadicore_dbus_xml ${Akonadi_SOURCE_DIR}/src/interfaces/org.freedesktop.Akonadi.NotificationManager.xml)
qt_add_dbus_interface(akonadicore_dbus_SRCS ${akonadicore_dbus_xml} notificationmanagerinterface)

set(akonadicore_dbus_xml ${Akonadi_SOURCE_DIR}/src/interfaces/org.freedesktop.Akonadi.NotificationSource.xml)
set_source_files_properties(${akonadicore_dbus_xml} PROPERTIES INCLUDE "${Akonadi_SOURCE_DIR}/src/private/protocol_p.h")
qt_add_dbus_interface(akonadicore_dbus_SRCS ${akonadicore_dbus_xml} notificationsourceinterface)

qt_add_dbus_interfaces(akonadicore_dbus_SRCS ${Akonadi_SOURCE_DIR}/src/interfaces/org.freedesktop.Akonadi.AgentManager.xml)
qt_add_dbus_interfaces(akonadicore_dbus_SRCS ${Akonadi_SOURCE_DIR}/src/interfaces/org.freedesktop.Akonadi.Tracer.xml)
qt_add_dbus_interfaces(akonadicore_dbus_SRCS ${Akonadi_SOURCE_DIR}/src/interfaces/org.freedesktop.Akonadi.Agent.Control.xml)
qt_add_dbus_interfaces(akonadicore_dbus_SRCS
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


add_library(KPim6AkonadiCore ${akonadicore_SRCS})
#if(COMPILE_WITH_UNITY_CMAKE_SUPPORT)
#    set_target_properties(KPim6AkonadiCore PROPERTIES UNITY_BUILD ON)
#endif()

generate_export_header(KPim6AkonadiCore BASE_NAME akonadicore)

add_library(KPim6::AkonadiCore ALIAS KPim6AkonadiCore)
target_include_directories(KPim6AkonadiCore INTERFACE "$<INSTALL_INTERFACE:${AkonadiCore_INSTALL_INCLUDEDIR}>")
target_include_directories(KPim6AkonadiCore PUBLIC "$<BUILD_INTERFACE:${Akonadi_SOURCE_DIR}/src/core;${Akonadi_BINARY_DIR}/src/core;${Akonadi_BINARY_DIR}/src/private>")
target_include_directories(KPim6AkonadiCore PUBLIC "$<BUILD_INTERFACE:${Akonadi_BINARY_DIR};${Akonadi_SOURCE_DIR}/src/core/jobs;${Akonadi_SOURCE_DIR}/src/core/models;${Akonadi_SOURCE_DIR}/src/core/attributes>")

kde_target_enable_exceptions(KPim6AkonadiCore PUBLIC)

target_link_libraries(KPim6AkonadiCore
PUBLIC
    KF6::CoreAddons # for KJob
    KF6::ItemModels
    Qt::Gui # for QColor
    KPim6::AkonadiPrivate
PRIVATE
    Qt::Network
    KF6::I18n
    KF6::IconThemes
    KF6::ConfigCore
    akonadi_shared
)

set_target_properties(KPim6AkonadiCore PROPERTIES
    VERSION ${AKONADI_VERSION}
    SOVERSION ${AKONADI_SOVERSION}
    EXPORT_NAME AkonadiCore
)

install(TARGETS
    KPim6AkonadiCore
    EXPORT KPim6AkonadiTargets ${KDE_INSTALL_TARGETS_DEFAULT_ARGS}
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
    DESTINATION ${AkonadiCore_INSTALL_INCLUDEDIR}/akonadi
    COMPONENT Devel
)

install(FILES
    ${AkonadiCore_base_CC_HEADERS}
    ${AkonadiCore_attributes_CC_HEADERS}
    ${AkonadiCore_models_CC_HEADERS}
    ${AkonadiCore_jobs_CC_HEADERS}
    ${AkonadiCore_CC_HEADERS}
    DESTINATION ${AkonadiCore_INSTALL_INCLUDEDIR}/Akonadi
    COMPONENT Devel
)

install(FILES
    kcfg2dbus.xsl
    DESTINATION ${KDE_INSTALL_DATADIR_KF}/akonadi
)


if(BUILD_QCH)
    ecm_add_qch(
        KPim6AkonadiCore_QCH
        NAME KPim6AkonadiCore
        BASE_NAME KPim6AkonadiCore
        VERSION ${PIM_VERSION}
        ORG_DOMAIN org.kde
        SOURCES # using only public headers, to cover only public API
        ${AkonadiCore_base_HEADERS}
        ${AkonadiCore_attributes_HEADERS}
        ${AkonadiCore_models_HEADERS}
        ${AkonadiCore_jobs_HEADERS}
        ${AkonadiCore_HEADERS}
        #MD_MAINPAGE "${CMAKE_SOURCE_DIR}/README.md"
        #IMAGE_DIRS "${CMAKE_SOURCE_DIR}/docs/pics"
        LINK_QCHS
            Qt6Core_QCH
            Qt6Gui_QCH
            Qt6Widgets_QCH
        INCLUDE_DIRS
            ${CMAKE_CURRENT_BINARY_DIR}
        BLANK_MACROS
            AKONADICORE_EXPORT
        TAGFILE_INSTALL_DESTINATION ${KDE_INSTALL_QTQCHDIR}
        QCH_INSTALL_DESTINATION ${KDE_INSTALL_QTQCHDIR}
        COMPONENT Devel
   )
endif()
