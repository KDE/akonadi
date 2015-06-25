include_directories(BEFORE ${CMAKE_CURRENT_BINARY_DIR})
include_directories(BEFORE ${CMAKE_CURRENT_SOURCE_DIR})

if(MYSQLD_EXECUTABLE)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DMYSQLD_EXECUTABLE=\"\\\"${MYSQLD_EXECUTABLE}\\\"\"")
endif()

if(POSTGRES_PATH)
  set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DPOSTGRES_PATH=\"\\\"${POSTGRES_PATH}\\\"\"")
endif()

########### next target ###############

set(AKONADI_DB_SCHEME ${CMAKE_CURRENT_SOURCE_DIR}/storage/akonadidb.xml)

add_custom_command(
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/entities.h
         ${CMAKE_CURRENT_BINARY_DIR}/entities.cpp
  COMMAND ${XSLTPROC_EXECUTABLE}
          --output ${CMAKE_CURRENT_BINARY_DIR}/entities.h
          --stringparam code header
          ${CMAKE_CURRENT_SOURCE_DIR}/storage/entities.xsl
          ${AKONADI_DB_SCHEME}
  COMMAND ${XSLTPROC_EXECUTABLE}
          --output ${CMAKE_CURRENT_BINARY_DIR}/entities.cpp
          --stringparam code source
          ${CMAKE_CURRENT_SOURCE_DIR}/storage/entities.xsl
          ${AKONADI_DB_SCHEME}
  DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/storage/entities.xsl
          ${CMAKE_CURRENT_SOURCE_DIR}/storage/entities-header.xsl
          ${CMAKE_CURRENT_SOURCE_DIR}/storage/entities-source.xsl
          ${AKONADI_DB_SCHEME}
)

add_test(akonadidb-xmllint ${XMLLINT_EXECUTABLE} --noout --schema ${CMAKE_CURRENT_SOURCE_DIR}/storage/akonadidb.xsd ${CMAKE_CURRENT_SOURCE_DIR}/storage/akonadidb.xml)
add_test(akonadidbupdate-xmllint ${XMLLINT_EXECUTABLE} --noout --schema ${CMAKE_CURRENT_SOURCE_DIR}/storage/dbupdate.xsd ${CMAKE_CURRENT_SOURCE_DIR}/storage/dbupdate.xml)

akonadi_generate_schema(${AKONADI_DB_SCHEME} AkonadiSchema akonadischema)

set(libakonadiserver_SRCS
    akonadi.cpp
    commandcontext.cpp
    connection.cpp
    connectionthread.cpp
    collectionscheduler.cpp
    dbusconnectionpool.cpp
    handler.cpp
    handlerhelper.cpp
    intervalcheck.cpp
    collectionreferencemanager.cpp
    handler/akappend.cpp
    handler/copy.cpp
    handler/colcopy.cpp
    handler/colmove.cpp
    handler/create.cpp
    handler/delete.cpp
    handler/fetch.cpp
    handler/fetchhelper.cpp
    handler/link.cpp
    handler/list.cpp
    handler/login.cpp
    handler/logout.cpp
    handler/modify.cpp
    handler/move.cpp
    handler/remove.cpp
    handler/resourceselect.cpp
    handler/relationstore.cpp
    handler/relationremove.cpp
    handler/relationfetch.cpp
    handler/search.cpp
    handler/searchhelper.cpp
    handler/searchpersistent.cpp
    handler/searchresult.cpp
    handler/select.cpp
    handler/status.cpp
    handler/store.cpp
    handler/tagappend.cpp
    handler/tagfetch.cpp
    handler/tagfetchhelper.cpp
    handler/tagremove.cpp
    handler/tagstore.cpp
    handler/transaction.cpp
    search/agentsearchengine.cpp
    search/agentsearchinstance.cpp
    search/searchtaskmanager.cpp
    search/searchtaskmanagerthread.cpp
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
    storage/itemretrievalthread.cpp
    storage/itemretrievaljob.cpp
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
    notificationsource.cpp
    resourcemanager.cpp
    cachecleaner.cpp
    debuginterface.cpp
    preprocessorinstance.cpp
    preprocessormanager.cpp
    storagejanitor.cpp
)

set(akonadiserver_SRCS
    main.cpp
    akonadiserver_debug.cpp
)

qt5_generate_dbus_interface(debuginterface.h org.freedesktop.Akonadi.DebugInterface.xml)

qt5_add_dbus_adaptor(libakonadiserver_SRCS ${Akonadi_SOURCE_DIR}/src/interfaces/org.freedesktop.Akonadi.TracerNotification.xml dbustracer.h Akonadi::Server::DBusTracer)
qt5_add_dbus_adaptor(libakonadiserver_SRCS ${Akonadi_SOURCE_DIR}/src/interfaces/org.freedesktop.Akonadi.Tracer.xml tracer.h Akonadi::Server::Tracer)
qt5_add_dbus_adaptor(libakonadiserver_SRCS ${Akonadi_SOURCE_DIR}/src/interfaces/org.freedesktop.Akonadi.NotificationManager.xml notificationmanager.h Akonadi::Server::NotificationManager)
qt5_add_dbus_adaptor(libakonadiserver_SRCS ${Akonadi_SOURCE_DIR}/src/interfaces/org.freedesktop.Akonadi.Server.xml akonadi.h Akonadi::Server::AkonadiServer)
qt5_add_dbus_adaptor(libakonadiserver_SRCS ${Akonadi_SOURCE_DIR}/src/interfaces/org.freedesktop.Akonadi.NotificationSource.xml notificationsource.h Akonadi::Server::NotificationSource)
qt5_add_dbus_adaptor(libakonadiserver_SRCS ${Akonadi_SOURCE_DIR}/src/interfaces/org.freedesktop.Akonadi.StorageDebugger.xml storage/storagedebugger.h Akonadi::Server::StorageDebugger)
qt5_add_dbus_adaptor(libakonadiserver_SRCS ${CMAKE_CURRENT_BINARY_DIR}/org.freedesktop.Akonadi.DebugInterface.xml debuginterface.h Akonadi::Server::DebugInterface)
qt5_add_dbus_adaptor(libakonadiserver_SRCS ${Akonadi_SOURCE_DIR}/src/interfaces/org.freedesktop.Akonadi.ResourceManager.xml resourcemanager.h Akonadi::Server::ResourceManager)
qt5_add_dbus_adaptor(libakonadiserver_SRCS ${Akonadi_SOURCE_DIR}/src/interfaces/org.freedesktop.Akonadi.PreprocessorManager.xml preprocessormanager.h Akonadi::Server::PreprocessorManager)
qt5_add_dbus_adaptor(libakonadiserver_SRCS ${Akonadi_SOURCE_DIR}/src/interfaces/org.freedesktop.Akonadi.SearchManager.xml search/searchmanager.h Akonadi::Server::SearchManager)
qt5_add_dbus_interfaces(libakonadiserver_SRCS ${Akonadi_SOURCE_DIR}/src/interfaces/org.freedesktop.Akonadi.AgentManager.xml ${Akonadi_SOURCE_DIR}/src/interfaces/org.freedesktop.Akonadi.Resource.xml)
qt5_add_dbus_interface(libakonadiserver_SRCS ${Akonadi_SOURCE_DIR}/src/interfaces/org.freedesktop.Akonadi.Preprocessor.xml preprocessorinterface)
qt5_add_dbus_interface(libakonadiserver_SRCS ${Akonadi_SOURCE_DIR}/src/interfaces/org.freedesktop.Akonadi.Agent.Control.xml agentcontrolinterface)
qt5_add_dbus_interface(libakonadiserver_SRCS ${Akonadi_SOURCE_DIR}/src/interfaces/org.freedesktop.Akonadi.Agent.Search.xml agentsearchinterface)

qt5_add_resources(libakonadiserver_SRCS storage/akonadidb.qrc)

add_library(libakonadiserver STATIC ${libakonadiserver_SRCS})
target_link_libraries(libakonadiserver
    akonadi_shared
    KF5AkonadiPrivate
    Qt5::Core
    Qt5::Network
    Qt5::Sql
    Qt5::DBus
    Qt5::Xml
)

add_executable(akonadiserver ${akonadiserver_SRCS})
set_target_properties(akonadiserver PROPERTIES OUTPUT_NAME akonadiserver)
target_link_libraries(akonadiserver
    libakonadiserver
)

install(TARGETS akonadiserver
        ${KF5_INSTALL_TARGETS_DEFAULT_ARGS}
)

install(FILES
  storage/mysql-global.conf
  storage/mysql-global-mobile.conf
  DESTINATION ${CONFIG_INSTALL_DIR}/akonadi
)

install(FILES
  search/abstractsearchplugin.h
  DESTINATION ${KF5_INCLUDE_INSTALL_DIR}/akonadi
)

## DBus XML files
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/org.freedesktop.Akonadi.DebugInterface.xml
        DESTINATION ${AKONADI_DBUS_INTERFACES_INSTALL_DIR}
)
