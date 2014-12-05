find_program(MYSQLD_EXECUTABLE NAMES mysqld
              PATHS /usr/sbin /usr/local/sbin /usr/libexec /usr/local/libexec /opt/mysql/libexec /usr/mysql/bin /opt/mysql/sbin
              DOC "The mysqld executable path. ONLY needed at runtime"
            )

if(MYSQLD_EXECUTABLE)
   message(STATUS "MySQL Server found: ${MYSQLD_EXECUTABLE}")
else()
   message(STATUS "MySQL Server wasn't found. it is required to use the MySQL backend.")
endif()

find_path(POSTGRES_PATH NAMES pg_ctl
              HINTS /usr/lib${LIB_SUFFIX}/postgresql/8.4/bin
                    /usr/lib${LIB_SUFFIX}/postgresql/9.0/bin
                    /usr/lib${LIB_SUFFIX}/postgresql/9.1/bin
              DOC "The pg_ctl executable path. ONLY needed at runtime by the PostgreSQL backend"
         )

if(POSTGRES_PATH)
   message(STATUS "PostgreSQL Server found.")
else()
   message(STATUS "PostgreSQL wasn't found. it is required to use the Postgres backend.")
endif()

include_directories(${Akonadi_SOURCE_DIR}/libs)
include_directories(${Akonadi_BINARY_DIR}/libs)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src/handler)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src/nepomuk)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/tests)
include_directories(${Akonadi_SOURCE_DIR}/shared)
if(Soprano_FOUND)
  include_directories(${SOPRANO_INCLUDE_DIR})
endif()
include_directories(BEFORE ${CMAKE_CURRENT_SOURCE_DIR})
include_directories(BEFORE ${CMAKE_CURRENT_BINARY_DIR})

macro(akonadi_generate_schema _schemaXml _className _fileBaseName)
add_custom_command(
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${_fileBaseName}.h
         ${CMAKE_CURRENT_BINARY_DIR}/${_fileBaseName}.cpp
  COMMAND ${XSLTPROC_EXECUTABLE}
          --output ${CMAKE_CURRENT_BINARY_DIR}/${_fileBaseName}.h
          --stringparam code header
          --stringparam className ${_className}
          --stringparam fileName ${_fileBaseName}
          ${Akonadi_SOURCE_DIR}/server/src/storage/schema.xsl
          ${_schemaXml}
  COMMAND ${XSLTPROC_EXECUTABLE}
          --output ${CMAKE_CURRENT_BINARY_DIR}/${_fileBaseName}.cpp
          --stringparam code source
          --stringparam className ${_className}
          --stringparam fileName ${_fileBaseName}
          ${Akonadi_SOURCE_DIR}/server/src/storage/schema.xsl
          ${_schemaXml}
  DEPENDS ${Akonadi_SOURCE_DIR}/server/src/storage/schema.xsl
          ${Akonadi_SOURCE_DIR}/server/src/storage/schema-header.xsl
          ${Akonadi_SOURCE_DIR}/server/src/storage/schema-source.xsl
          ${_schemaXml}
)
endmacro()


add_subdirectory(akonadictl)
add_subdirectory(control)
add_subdirectory(src)

if(AKONADI_BUILD_TESTS)
  add_subdirectory(tests)
endif()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${_ENABLE_EXCEPTIONS}")
if(MYSQLD_EXECUTABLE)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DMYSQLD_EXECUTABLE=\"\\\"${MYSQLD_EXECUTABLE}\\\"\"")
endif()

if(POSTGRES_PATH)
  set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DPOSTGRES_PATH=\"\\\"${POSTGRES_PATH}\\\"\"")
endif()

########### next target ###############

set(AKONADI_DB_SCHEME ${CMAKE_CURRENT_SOURCE_DIR}/src/storage/akonadidb.xml)

add_custom_command(
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/entities.h
         ${CMAKE_CURRENT_BINARY_DIR}/entities.cpp
  COMMAND ${XSLTPROC_EXECUTABLE}
          --output ${CMAKE_CURRENT_BINARY_DIR}/entities.h
          --stringparam code header
          ${CMAKE_CURRENT_SOURCE_DIR}/src/storage/entities.xsl
          ${AKONADI_DB_SCHEME}
  COMMAND ${XSLTPROC_EXECUTABLE}
          --output ${CMAKE_CURRENT_BINARY_DIR}/entities.cpp
          --stringparam code source
          ${CMAKE_CURRENT_SOURCE_DIR}/src/storage/entities.xsl
          ${AKONADI_DB_SCHEME}
  DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/src/storage/entities.xsl
          ${CMAKE_CURRENT_SOURCE_DIR}/src/storage/entities-header.xsl
          ${CMAKE_CURRENT_SOURCE_DIR}/src/storage/entities-source.xsl
          ${AKONADI_DB_SCHEME}
)

akonadi_generate_schema(${AKONADI_DB_SCHEME} AkonadiSchema akonadischema)

set(libakonadiprivate_SRCS
  src/akonadi.cpp
  src/commandcontext.cpp
  src/connection.cpp
  src/connectionthread.cpp
  src/collectionscheduler.cpp
  src/clientcapabilities.cpp
  src/clientcapabilityaggregator.cpp
  src/dbusconnectionpool.cpp
  src/handler.cpp
  src/handlerhelper.cpp
  src/intervalcheck.cpp
  src/response.cpp
  src/collectionreferencemanager.cpp
  src/handler/akappend.cpp
  src/handler/append.cpp
  src/handler/copy.cpp
  src/handler/colcopy.cpp
  src/handler/colmove.cpp
  src/handler/create.cpp
  src/handler/capability.cpp
  src/handler/delete.cpp
  src/handler/expunge.cpp
  src/handler/fetch.cpp
  src/handler/fetchhelper.cpp
  src/handler/fetchscope.cpp
  src/handler/link.cpp
  src/handler/list.cpp
  src/handler/login.cpp
  src/handler/logout.cpp
  src/handler/merge.cpp
  src/handler/modify.cpp
  src/handler/move.cpp
  src/handler/remove.cpp
  src/handler/resourceselect.cpp
  src/handler/scope.cpp
  src/handler/search.cpp
  src/handler/searchhelper.cpp
  src/handler/searchpersistent.cpp
  src/handler/searchresult.cpp
  src/handler/select.cpp
  src/handler/subscribe.cpp
  src/handler/status.cpp
  src/handler/store.cpp
  src/handler/tagappend.cpp
  src/handler/tagfetch.cpp
  src/handler/tagfetchhelper.cpp
  src/handler/tagremove.cpp
  src/handler/tagstore.cpp
  src/handler/transaction.cpp
  src/search/agentsearchengine.cpp
  src/search/agentsearchinstance.cpp
  src/search/searchtaskmanager.cpp
  src/search/searchtaskmanagerthread.cpp
  src/search/searchrequest.cpp
  src/search/searchmanager.cpp

  src/storage/collectionqueryhelper.cpp
  src/storage/collectionstatistics.cpp
  src/storage/entity.cpp
  ${CMAKE_CURRENT_BINARY_DIR}/entities.cpp
  ${CMAKE_CURRENT_BINARY_DIR}/akonadischema.cpp
  src/storage/datastore.cpp
  src/storage/dbconfig.cpp
  src/storage/dbconfigmysql.cpp
  src/storage/dbconfigpostgresql.cpp
  src/storage/dbconfigsqlite.cpp
  src/storage/dbexception.cpp
  src/storage/dbinitializer.cpp
  src/storage/dbinitializer_p.cpp
  src/storage/dbintrospector.cpp
  src/storage/dbintrospector_impl.cpp
  src/storage/dbupdater.cpp
  src/storage/dbtype.cpp
  src/storage/itemqueryhelper.cpp
  src/storage/itemretriever.cpp
  src/storage/itemretrievalmanager.cpp
  src/storage/itemretrievalthread.cpp
  src/storage/itemretrievaljob.cpp
  src/storage/notificationcollector.cpp
  src/storage/parthelper.cpp
  src/storage/parttypehelper.cpp
  src/storage/query.cpp
  src/storage/querybuilder.cpp
  src/storage/querycache.cpp
  src/storage/queryhelper.cpp
  src/storage/schematypes.cpp
  src/storage/transaction.cpp
  src/storage/parthelper.cpp
  src/storage/partstreamer.cpp
  src/storage/storagedebugger.cpp
  src/tracer.cpp
  src/utils.cpp
  src/dbustracer.cpp
  src/filetracer.cpp
  src/notificationmanager.cpp
  src/notificationsource.cpp
  src/resourcemanager.cpp
  src/cachecleaner.cpp
  src/debuginterface.cpp
  src/imapstreamparser.cpp
  src/preprocessorinstance.cpp
  src/preprocessormanager.cpp
  src/storagejanitor.cpp
)

if (Soprano_FOUND)
  set(libakonadiprivate_SRCS
    ${libakonadiprivate_SRCS}
    src/nepomuksearch.cpp
    src/search/nepomuksearchengine.cpp
    src/nepomuk/dbusoperators.cpp
    src/nepomuk/queryserviceclient.cpp
    src/nepomuk/result.cpp
  )

  set_source_files_properties(src/nepomuk/org.kde.nepomuk.QueryService.xml PROPERTIES INCLUDE "dbusoperators.h")
  set_source_files_properties(src/nepomuk/org.kde.nepomuk.Query.xml PROPERTIES INCLUDE "result.h")

  qt4_add_dbus_interface(libakonadiprivate_SRCS src/nepomuk/org.kde.nepomuk.QueryService.xml queryserviceinterface)
  qt4_add_dbus_interface(libakonadiprivate_SRCS src/nepomuk/org.kde.nepomuk.Query.xml queryinterface)
endif()

qt4_generate_dbus_interface(src/debuginterface.h org.freedesktop.Akonadi.DebugInterface.xml)

qt4_add_dbus_adaptor(libakonadiprivate_SRCS ${Akonadi_SOURCE_DIR}/interfaces/org.freedesktop.Akonadi.TracerNotification.xml dbustracer.h Akonadi::Server::DBusTracer)
qt4_add_dbus_adaptor(libakonadiprivate_SRCS ${Akonadi_SOURCE_DIR}/interfaces/org.freedesktop.Akonadi.Tracer.xml tracer.h Akonadi::Server::Tracer)
qt4_add_dbus_adaptor(libakonadiprivate_SRCS ${Akonadi_SOURCE_DIR}/interfaces/org.freedesktop.Akonadi.NotificationManager.xml notificationmanager.h Akonadi::Server::NotificationManager)
qt4_add_dbus_adaptor(libakonadiprivate_SRCS ${Akonadi_SOURCE_DIR}/interfaces/org.freedesktop.Akonadi.Server.xml akonadi.h Akonadi::Server::AkonadiServer)
qt4_add_dbus_adaptor(libakonadiprivate_SRCS ${Akonadi_SOURCE_DIR}/interfaces/org.freedesktop.Akonadi.NotificationSource.xml notificationsource.h Akonadi::Server::NotificationSource)
qt4_add_dbus_adaptor(libakonadiprivate_SRCS ${Akonadi_SOURCE_DIR}/interfaces/org.freedesktop.Akonadi.StorageDebugger.xml storage/storagedebugger.h Akonadi::Server::StorageDebugger)
qt4_add_dbus_adaptor(libakonadiprivate_SRCS ${CMAKE_CURRENT_BINARY_DIR}/org.freedesktop.Akonadi.DebugInterface.xml debuginterface.h Akonadi::Server::DebugInterface)
qt4_add_dbus_adaptor(libakonadiprivate_SRCS ${Akonadi_SOURCE_DIR}/interfaces/org.freedesktop.Akonadi.ResourceManager.xml resourcemanager.h Akonadi::Server::ResourceManager)
qt4_add_dbus_adaptor(libakonadiprivate_SRCS ${Akonadi_SOURCE_DIR}/interfaces/org.freedesktop.Akonadi.PreprocessorManager.xml preprocessormanager.h Akonadi::Server::PreprocessorManager)
qt4_add_dbus_adaptor(libakonadiprivate_SRCS ${Akonadi_SOURCE_DIR}/interfaces/org.freedesktop.Akonadi.SearchManager.xml search/searchmanager.h Akonadi::Server::SearchManager)
qt4_add_dbus_interfaces(libakonadiprivate_SRCS ${Akonadi_SOURCE_DIR}/interfaces/org.freedesktop.Akonadi.AgentManager.xml ${Akonadi_SOURCE_DIR}/interfaces/org.freedesktop.Akonadi.Resource.xml)
qt4_add_dbus_interface(libakonadiprivate_SRCS ${Akonadi_SOURCE_DIR}/interfaces/org.freedesktop.Akonadi.Preprocessor.xml preprocessorinterface)
qt4_add_dbus_interface(libakonadiprivate_SRCS ${Akonadi_SOURCE_DIR}/interfaces/org.freedesktop.Akonadi.Agent.Control.xml agentcontrolinterface)
qt4_add_dbus_interface(libakonadiprivate_SRCS ${Akonadi_SOURCE_DIR}/interfaces/org.freedesktop.Akonadi.Agent.Search.xml agentsearchinterface)

qt4_add_resources(libakonadiprivate_SRCS src/storage/akonadidb.qrc)


add_library(akonadiprivate STATIC ${libakonadiprivate_SRCS})

if(QT5_BUILD)
  # TODO: Remove when Qt 4 is no longer supported.
  set_property(TARGET akonadiprivate APPEND PROPERTY COMPILE_DEFINITIONS QT_DISABLE_DEPRECATED_BEFORE=0)
endif()

target_link_libraries(akonadiprivate
  akonadi_shared
  ${QT_QTCORE_LIBRARY}
  ${QT_QTNETWORK_LIBRARY}
  ${QT_QTSQL_LIBRARY}
  ${QT_QTDBUS_LIBRARY}
  ${QT_QTXML_LIBRARY}
  ${AKONADI_PROTOCOLINTERNALS_LIBS}
  ${AKONADI_SYSTEM_LIBS}
  ${Boost_PROGRAM_OPTIONS_LIBRARY}
)

if(Soprano_FOUND)
  target_link_libraries(akonadiprivate ${SOPRANO_LIBRARIES})
endif()

set(akonadiserver_bin_SRCS src/main.cpp)

if(AKONADI_STATIC_SQLITE)
  add_definitions(-DQT_STATICPLUGIN)
endif()

add_executable(akonadiserver_bin ${akonadiserver_bin_SRCS})
set_target_properties(akonadiserver_bin PROPERTIES OUTPUT_NAME akonadiserver)
target_link_libraries(akonadiserver_bin akonadi_shared akonadiprivate ${QT_QTCORE_LIBRARY})
if(AKONADI_STATIC_SQLITE)
  target_link_libraries(akonadiserver_bin qsqlite3)
endif()

install(TARGETS akonadiserver_bin DESTINATION ${BIN_INSTALL_DIR})
install(FILES
  src/storage/mysql-global.conf
  src/storage/mysql-global-mobile.conf
  DESTINATION ${CONFIG_INSTALL_DIR}/akonadi)

install(FILES
  src/search/abstractsearchplugin.h
  DESTINATION ${INCLUDE_INSTALL_DIR}/akonadi)

## DBus XML files
install(FILES
   ${CMAKE_CURRENT_BINARY_DIR}/org.freedesktop.Akonadi.DebugInterface.xml
DESTINATION ${DBUS_INTERFACES_INSTALL_DIR})
