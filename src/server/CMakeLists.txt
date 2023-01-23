########### next target ###############
set(AKONADI_DB_SCHEMA "${CMAKE_CURRENT_SOURCE_DIR}/storage/akonadidb.xml")

akonadi_run_xsltproc(
    XSL ${CMAKE_CURRENT_SOURCE_DIR}/storage/entities.xsl
    XML ${AKONADI_DB_SCHEMA}
    BASENAME entities
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/storage/entities-source.xsl ${CMAKE_CURRENT_SOURCE_DIR}/storage/entities-header.xsl
)

akonadi_run_xsltproc(
    XSL ${Akonadi_SOURCE_DIR}/src/server/storage/schema.xsl
    XML ${AKONADI_DB_SCHEMA}
    CLASSNAME AkonadiSchema
    BASENAME akonadischema
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/storage/schema-source.xsl ${CMAKE_CURRENT_SOURCE_DIR}/storage/schema-header.xsl
)

akonadi_add_xmllint_test(
    akonadidb-xmllint
    XSD ${CMAKE_CURRENT_SOURCE_DIR}/storage/akonadidb.xsd
    XML ${AKONADI_DB_SCHEMA}
)
akonadi_add_xmllint_test(
    akonadidbupdate-xmllint
    XSD ${CMAKE_CURRENT_SOURCE_DIR}/storage/akonadidb.xsd
    XML ${AKONADI_DB_SCHEMA}
)

set(libakonadiserver_SRCS
    akonadi.cpp
    aggregatedfetchscope.cpp
    aklocalserver.cpp
    akthread.cpp
    commandcontext.cpp
    connection.cpp
    collectionscheduler.cpp
    handler.cpp
    handlerhelper.cpp
    intervalcheck.cpp
    handler/collectioncopyhandler.cpp
    handler/collectioncreatehandler.cpp
    handler/collectiondeletehandler.cpp
    handler/collectionfetchhandler.cpp
    handler/collectionmodifyhandler.cpp
    handler/collectionmovehandler.cpp
    handler/collectionstatsfetchhandler.cpp
    handler/itemcopyhandler.cpp
    handler/itemcreatehandler.cpp
    handler/itemdeletehandler.cpp
    handler/itemfetchhandler.cpp
    handler/itemfetchhelper.cpp
    handler/itemlinkhandler.cpp
    handler/itemmodifyhandler.cpp
    handler/itemmovehandler.cpp
    handler/loginhandler.cpp
    handler/logouthandler.cpp
    handler/relationfetchhandler.cpp
    handler/relationmodifyhandler.cpp
    handler/relationremovehandler.cpp
    handler/resourceselecthandler.cpp
    handler/searchhandler.cpp
    handler/searchhelper.cpp
    handler/searchcreatehandler.cpp
    handler/searchresulthandler.cpp
    handler/tagcreatehandler.cpp
    handler/tagdeletehandler.cpp
    handler/tagfetchhandler.cpp
    handler/tagfetchhelper.cpp
    handler/tagmodifyhandler.cpp
    handler/transactionhandler.cpp
    search/agentsearchengine.cpp
    search/agentsearchinstance.cpp
    search/searchtaskmanager.cpp
    search/searchrequest.cpp
    search/searchmanager.cpp

    storage/collectionqueryhelper.cpp
    storage/collectionstatistics.cpp
    storage/entity.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/entities.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/akonadischema.cpp
    storage/datastore.cpp
    storage/dbconfig.cpp
    storage/dbconfigmysql.cpp
    storage/dbconfigpostgresql.cpp
    storage/dbconfigsqlite.cpp
    storage/dbexception.cpp
    storage/dbinitializer.cpp
    storage/dbinitializer_p.cpp
    storage/dbintrospector.cpp
    storage/dbintrospector_impl.cpp
    storage/dbupdater.cpp
    storage/dbtype.cpp
    storage/itemqueryhelper.cpp
    storage/itemretriever.cpp
    storage/itemretrievalmanager.cpp
    storage/itemretrievaljob.cpp
    storage/itemretrievalrequest.cpp
    storage/notificationcollector.cpp
    storage/parthelper.cpp
    storage/parttypehelper.cpp
    storage/query.cpp
    storage/querybuilder.cpp
    storage/querycache.cpp
    storage/queryhelper.cpp
    storage/schematypes.cpp
    storage/tagqueryhelper.cpp
    storage/transaction.cpp
    storage/parthelper.cpp
    storage/partstreamer.cpp
    storage/storagedebugger.cpp
    tracer.cpp
    utils.cpp
    dbustracer.cpp
    filetracer.cpp
    notificationmanager.cpp
    notificationsubscriber.cpp
    resourcemanager.cpp
    cachecleaner.cpp
    debuginterface.cpp
    preprocessorinstance.cpp
    preprocessormanager.cpp
    storagejanitor.cpp
    storage/akonadidb.qrc


    akonadi.h
    aggregatedfetchscope.h
    aklocalserver.h
    akthread.h
    commandcontext.h
    connection.h
    collectionscheduler.h
    handler.h
    handlerhelper.h
    intervalcheck.h
    handler/collectioncopyhandler.h
    handler/collectioncreatehandler.h
    handler/collectiondeletehandler.h
    handler/collectionfetchhandler.h
    handler/collectionmodifyhandler.h
    handler/collectionmovehandler.h
    handler/collectionstatsfetchhandler.h
    handler/itemcopyhandler.h
    handler/itemcreatehandler.h
    handler/itemdeletehandler.h
    handler/itemfetchhandler.h
    handler/itemfetchhelper.h
    handler/itemlinkhandler.h
    handler/itemmodifyhandler.h
    handler/itemmovehandler.h
    handler/loginhandler.h
    handler/logouthandler.h
    handler/relationfetchhandler.h
    handler/relationmodifyhandler.h
    handler/relationremovehandler.h
    handler/resourceselecthandler.h
    handler/searchhandler.h
    handler/searchhelper.h
    handler/searchcreatehandler.h
    handler/searchresulthandler.h
    handler/tagcreatehandler.h
    handler/tagdeletehandler.h
    handler/tagfetchhandler.h
    handler/tagfetchhelper.h
    handler/tagmodifyhandler.h
    handler/transactionhandler.h
    search/agentsearchengine.h
    search/agentsearchinstance.h
    search/searchtaskmanager.h
    search/searchrequest.h
    search/searchmanager.h
    search/abstractsearchplugin.h

    storage/collectionqueryhelper.h
    storage/collectionstatistics.h
    storage/entity.h
    storage/datastore.h
    storage/dbconfig.h
    storage/dbconfigmysql.h
    storage/dbconfigpostgresql.h
    storage/dbconfigsqlite.h
    storage/dbexception.h
    storage/dbinitializer.h
    storage/dbinitializer_p.h
    storage/dbintrospector.h
    storage/dbintrospector_impl.h
    storage/dbupdater.h
    storage/dbtype.h
    storage/itemqueryhelper.h
    storage/itemretriever.h
    storage/itemretrievalmanager.h
    storage/itemretrievaljob.h
    storage/itemretrievalrequest.h
    storage/notificationcollector.h
    storage/parthelper.h
    storage/parttypehelper.h
    storage/query.h
    storage/querybuilder.h
    storage/querycache.h
    storage/queryhelper.h
    storage/schematypes.h
    storage/tagqueryhelper.h
    storage/transaction.h
    storage/parthelper.h
    storage/partstreamer.h
    storage/storagedebugger.h
    tracer.h
    utils.h
    dbustracer.h
    filetracer.h
    notificationmanager.h
    notificationsubscriber.h
    resourcemanager.h
    cachecleaner.h
    debuginterface.h
    preprocessorinstance.h
    preprocessormanager.h
    storagejanitor.h

    )

set(akonadiserver_SRCS
    main.cpp
)
ecm_qt_declare_logging_category(akonadiserver_SRCS HEADER akonadiserver_debug.h IDENTIFIER AKONADISERVER_LOG CATEGORY_NAME org.kde.pim.akonadiserver
        DESCRIPTION "akonadi (Akonadi Server)"
        OLD_CATEGORY_NAMES log_akonadiserver
        EXPORT AKONADI
    )

ecm_qt_declare_logging_category(akonadiserver_SRCS HEADER akonadiserver_search_debug.h IDENTIFIER AKONADISERVER_SEARCH_LOG CATEGORY_NAME org.kde.pim.akonadiserver.search
        DESCRIPTION "akonadi (Akonadi Server Search Functionality)"
        EXPORT AKONADI
    )

qt_generate_dbus_interface(debuginterface.h org.freedesktop.Akonadi.DebugInterface.xml)

qt_add_dbus_adaptor(libakonadiserver_SRCS ${Akonadi_SOURCE_DIR}/src/interfaces/org.freedesktop.Akonadi.TracerNotification.xml dbustracer.h Akonadi::Server::DBusTracer)
qt_add_dbus_adaptor(libakonadiserver_SRCS ${Akonadi_SOURCE_DIR}/src/interfaces/org.freedesktop.Akonadi.Tracer.xml tracer.h Akonadi::Server::Tracer)
qt_add_dbus_adaptor(libakonadiserver_SRCS ${Akonadi_SOURCE_DIR}/src/interfaces/org.freedesktop.Akonadi.Server.xml akonadi.h Akonadi::Server::AkonadiServer)
qt_add_dbus_adaptor(libakonadiserver_SRCS ${Akonadi_SOURCE_DIR}/src/interfaces/org.freedesktop.Akonadi.StorageDebugger.xml storage/storagedebugger.h Akonadi::Server::StorageDebugger)
qt_add_dbus_adaptor(libakonadiserver_SRCS ${CMAKE_CURRENT_BINARY_DIR}/org.freedesktop.Akonadi.DebugInterface.xml debuginterface.h Akonadi::Server::DebugInterface)
qt_add_dbus_adaptor(libakonadiserver_SRCS ${Akonadi_SOURCE_DIR}/src/interfaces/org.freedesktop.Akonadi.ResourceManager.xml resourcemanager.h Akonadi::Server::ResourceManager)
qt_add_dbus_adaptor(libakonadiserver_SRCS ${Akonadi_SOURCE_DIR}/src/interfaces/org.freedesktop.Akonadi.PreprocessorManager.xml preprocessormanager.h Akonadi::Server::PreprocessorManager)
qt_add_dbus_interface(libakonadiserver_SRCS ${Akonadi_SOURCE_DIR}/src/interfaces/org.freedesktop.Akonadi.AgentManager.xml agentmanagerinterface)
qt_add_dbus_interface(libakonadiserver_SRCS ${Akonadi_SOURCE_DIR}/src/interfaces/org.freedesktop.Akonadi.Resource.xml resourceinterface)
qt_add_dbus_interface(libakonadiserver_SRCS ${Akonadi_SOURCE_DIR}/src/interfaces/org.freedesktop.Akonadi.Preprocessor.xml preprocessorinterface)
qt_add_dbus_interface(libakonadiserver_SRCS ${Akonadi_SOURCE_DIR}/src/interfaces/org.freedesktop.Akonadi.Agent.Control.xml agentcontrolinterface)
qt_add_dbus_interface(libakonadiserver_SRCS ${Akonadi_SOURCE_DIR}/src/interfaces/org.freedesktop.Akonadi.Agent.Search.xml agentsearchinterface)

add_library(libakonadiserver STATIC ${libakonadiserver_SRCS})
#if (COMPILE_WITH_UNITY_CMAKE_SUPPORT)
#    set_target_properties(libakonadiserver PROPERTIES UNITY_BUILD ON)
#    set_source_files_properties(storage/itemretrievalmanager.cpp PROPERTIES SKIP_UNITY_BUILD_INCLUSION ON)
#endif()

set_target_properties(libakonadiserver PROPERTIES OUTPUT_NAME akonadiserver)

if(MYSQLD_EXECUTABLE)
  target_compile_definitions(libakonadiserver PRIVATE MYSQLD_EXECUTABLE="${MYSQLD_EXECUTABLE}")
endif()

if(MYSQLD_SCRIPTS_PATH)
  target_compile_definitions(libakonadiserver PRIVATE MYSQLD_SCRIPTS_PATH="${MYSQLD_SCRIPTS_PATH}")
endif()

if(POSTGRES_PATH)
  target_compile_definitions(libakonadiserver PRIVATE POSTGRES_PATH="${POSTGRES_PATH}")
endif()

target_link_libraries(libakonadiserver
    akonadi_shared
    KF5AkonadiPrivate
    Qt::Core
    Qt::Network
    Qt::Sql
    Qt::DBus
    Qt::Xml
)

add_executable(akonadiserver ${akonadiserver_SRCS})
set_target_properties(akonadiserver PROPERTIES MACOSX_BUNDLE FALSE)
set_target_properties(akonadiserver PROPERTIES OUTPUT_NAME akonadiserver)
target_link_libraries(akonadiserver
    libakonadiserver
    KF${KF_MAJOR_VERSION}::CoreAddons
)

install(TARGETS akonadiserver
        ${KF_INSTALL_TARGETS_DEFAULT_ARGS}
)

install(FILES
  storage/mysql-global.conf
  storage/mysql-global-mobile.conf
  DESTINATION ${KDE_INSTALL_CONFDIR}/akonadi
)

install(FILES
  search/abstractsearchplugin.h
  DESTINATION ${KDE_INSTALL_INCLUDEDIR_KF}/AkonadiCore/akonadi
)

## DBus XML files
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/org.freedesktop.Akonadi.DebugInterface.xml
        DESTINATION ${AKONADI_DBUS_INTERFACES_INSTALL_DIR}
)
