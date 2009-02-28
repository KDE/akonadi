# disable MySQL/Embedded plugin for now since it is unused and causes linker problems on Debian based 64bit systems
# macro_optional_find_package(MySQL)
# macro_log_feature(MYSQL_EMBEDDED_FOUND "mysql-embedded" "MySQL/Embedded Library" "http://www.mysql.org" FALSE "" "Needed for the MySQL/Embedded Akonadi backend. Sqlite will be used instead.")
#
# macro_optional_find_package(OpenSSL)
# macro_log_feature(OPENSSL_FOUND "OpenSSL" "A toolkit implementing the Secure Sockets Layer (SSL v2/v3) and Transport Layer Security (TLS v1) protocols" "http://openssl.org" FALSE "" "Required for building the Akonadi SQL plugin with MySQL embedded.")


# according to akonadi/CMakeLists.txt still cmake 2.4.5 is required,
# which doesn't have this command, so only do it if it exists. Alex
if(COMMAND cmake_policy)
   cmake_policy(SET CMP0003 NEW)
endif(COMMAND cmake_policy)

find_program( MYSQLD_EXECUTABLE mysqld /usr/sbin /usr/local/sbin /usr/libexec /opt/mysql/libexec )
macro_log_feature( MYSQLD_EXECUTABLE "MySQL Server" "Database server" "http://www.mysql.com" FALSE ""
                   "Akonadi server requires the mysqld binary" )

include_directories( ${CMAKE_CURRENT_BINARY_DIR} )
include_directories( ${CMAKE_CURRENT_SOURCE_DIR} )
include_directories( ${Boost_INCLUDE_DIR} )
include_directories( ${CMAKE_CURRENT_SOURCE_DIR}/../libs )
include_directories( ${CMAKE_CURRENT_BINARY_DIR}/../libs )
include_directories( ${CMAKE_CURRENT_SOURCE_DIR}/src )
include_directories( ${CMAKE_CURRENT_SOURCE_DIR}/src/handler )
include_directories( ${CMAKE_CURRENT_SOURCE_DIR}/tests )
include_directories( ${CMAKE_CURRENT_SOURCE_DIR}/shared )

set( AKONADI_PROTOCOLINTERNALS_LIBS ${akonadiprotocolinternals_LIB_DEPENDS} akonadiprotocolinternals)

set( AKONADI_SHARED_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/shared/akapplication.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/shared/akcrash.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/shared/akdebug.cpp
)

add_subdirectory( akonadictl )
add_subdirectory( control )
add_subdirectory( src )
#add_subdirectory( queryserver )
add_subdirectory( tests )

set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${_ENABLE_EXCEPTIONS} -DQT_NO_CAST_FROM_ASCII -DQT_NO_CAST_TO_ASCII" )
if ( MYSQLD_EXECUTABLE )
  set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DMYSQLD_EXECUTABLE=\"\\\"${MYSQLD_EXECUTABLE}\\\"\"" )
endif ( MYSQLD_EXECUTABLE )

########### next target ###############

add_custom_command(
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/entities.h
         ${CMAKE_CURRENT_BINARY_DIR}/entities.cpp
  COMMAND ${XSLTPROC_EXECUTABLE} --stringparam code header
          ${CMAKE_CURRENT_SOURCE_DIR}/src/storage/entities.xsl
          ${CMAKE_CURRENT_SOURCE_DIR}/src/storage/akonadidb.xml
          > ${CMAKE_CURRENT_BINARY_DIR}/entities.h
  COMMAND ${XSLTPROC_EXECUTABLE} --stringparam code source
          ${CMAKE_CURRENT_SOURCE_DIR}/src/storage/entities.xsl
          ${CMAKE_CURRENT_SOURCE_DIR}/src/storage/akonadidb.xml
          > ${CMAKE_CURRENT_BINARY_DIR}/entities.cpp
  DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/src/storage/entities.xsl
          ${CMAKE_CURRENT_SOURCE_DIR}/src/storage/entities-header.xsl
          ${CMAKE_CURRENT_SOURCE_DIR}/src/storage/entities-source.xsl
          ${CMAKE_CURRENT_SOURCE_DIR}/src/storage/akonadidb.xml
)

set(libakonadiprivate_SRCS
  ${AKONADI_SHARED_SOURCES}
  src/akonadi.cpp
  src/akonadiconnection.cpp
  src/handler.cpp
  src/handlerhelper.cpp
  src/intervalcheck.cpp
  src/response.cpp
  src/handler/aklist.cpp
  src/handler/akappend.cpp
  src/handler/append.cpp
  src/handler/copy.cpp
  src/handler/colcopy.cpp
  src/handler/create.cpp
  src/handler/capability.cpp
  src/handler/delete.cpp
  src/handler/expunge.cpp
  src/handler/fetch.cpp
  src/handler/link.cpp
  src/handler/list.cpp
  src/handler/login.cpp
  src/handler/logout.cpp
  src/handler/modify.cpp
  src/handler/noop.cpp
  src/handler/rename.cpp
  src/handler/searchhelper.cpp
  src/handler/searchpersistent.cpp
  src/handler/select.cpp
  src/handler/subscribe.cpp
  src/handler/status.cpp
  src/handler/store.cpp
  src/handler/transaction.cpp
  src/handler/uid.cpp
  src/storage/entity.cpp
  ${CMAKE_CURRENT_BINARY_DIR}/entities.cpp
  src/storage/datastore.cpp
  src/storage/dbconfig.cpp
  src/storage/dbinitializer.cpp
  src/storage/dbupdater.cpp
  src/storage/itemqueryhelper.cpp
  src/storage/itemretriever.cpp
  src/storage/itemretrievalmanager.cpp
  src/storage/itemretrievalthread.cpp
  src/storage/notificationcollector.cpp
  src/storage/query.cpp
  src/storage/querybuilder.cpp
  src/storage/transaction.cpp
  src/storage/parthelper.cpp
  src/tracer.cpp
  src/dbustracer.cpp
  src/filetracer.cpp
  src/notificationmanager.cpp
  src/resourcemanager.cpp
  src/cachecleaner.cpp
  src/abstractsearchmanager.cpp
  src/xesammanager.cpp
  src/nepomukmanager.cpp
  src/debuginterface.cpp
  src/imapstreamparser.cpp
)

qt4_generate_dbus_interface( src/debuginterface.h org.freedesktop.Akonadi.DebugInterface.xml )

qt4_add_dbus_adaptor( libakonadiprivate_SRCS interfaces/org.freedesktop.Akonadi.TracerNotification.xml dbustracer.h Akonadi::DBusTracer )
qt4_add_dbus_adaptor( libakonadiprivate_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/../interfaces/org.freedesktop.Akonadi.Tracer.xml tracer.h Akonadi::Tracer
)
qt4_add_dbus_adaptor( libakonadiprivate_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/../interfaces/org.freedesktop.Akonadi.NotificationManager.xml notificationmanager.h Akonadi::NotificationManager )
qt4_add_dbus_adaptor( libakonadiprivate_SRCS interfaces/org.freedesktop.Akonadi.Server.xml akonadi.h Akonadi::AkonadiServer )
qt4_add_dbus_adaptor( libakonadiprivate_SRCS ${CMAKE_CURRENT_BINARY_DIR}/org.freedesktop.Akonadi.DebugInterface.xml debuginterface.h DebugInterface )

qt4_add_dbus_interfaces( libakonadiprivate_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/../interfaces/org.freedesktop.Akonadi.AgentManager.xml ${CMAKE_CURRENT_SOURCE_DIR}/../interfaces/org.freedesktop.Akonadi.Resource.xml )
qt4_add_dbus_interface( libakonadiprivate_SRCS interfaces/org.freedesktop.Akonadi.Search.xml searchinterface )
qt4_add_dbus_interface( libakonadiprivate_SRCS interfaces/org.freedesktop.Akonadi.SearchQuery.xml searchqueryinterface )
qt4_add_dbus_interface( libakonadiprivate_SRCS interfaces/org.freedesktop.Akonadi.SearchQueryIterator.xml searchqueryiteratorinterface )

set(xesam_xml  interfaces/org.freedesktop.xesam.Search.xml)

set_source_files_properties(${xesam_xml} PROPERTIES INCLUDE "xesamtypes.h")

qt4_add_dbus_interface( libakonadiprivate_SRCS ${xesam_xml} xesaminterface )

qt4_add_resources( libakonadiprivate_SRCS src/storage/akonadidb.qrc )


automoc4_add_library( akonadiprivate SHARED ${libakonadiprivate_SRCS} )
target_link_libraries( akonadiprivate
  ${QT_QTCORE_LIBRARY}
  ${QT_QTNETWORK_LIBRARY}
  ${QT_QTSQL_LIBRARY}
  ${QT_QTDBUS_LIBRARY}
  ${QT_QTXML_LIBRARY}
  ${AKONADI_PROTOCOLINTERNALS_LIBS}
  ${Boost_PROGRAM_OPTIONS_LIBRARY}
)

set_target_properties( akonadiprivate PROPERTIES
                        VERSION ${AKONADI_LIB_VERSION_STRING}
                        SOVERSION ${AKONADI_LIB_MAJOR_VERSION}
                        DEFINE_SYMBOL MAKE_AKONADIPRIVATE_LIB
                      )

install( TARGETS akonadiprivate ${INSTALL_TARGETS_DEFAULT_ARGS} )

set( akonadiserver_bin_SRCS src/main.cpp
  ${AKONADI_SHARED_SOURCES} )


add_executable( akonadiserver_bin ${akonadiserver_bin_SRCS} )
set_target_properties( akonadiserver_bin PROPERTIES OUTPUT_NAME akonadiserver )
target_link_libraries( akonadiserver_bin akonadiprivate )

install( TARGETS akonadiserver_bin DESTINATION ${BIN_INSTALL_DIR} )
install( FILES src/storage/mysql-global.conf DESTINATION ${CONFIG_INSTALL_DIR}/akonadi/ )

## DBus XML files

install( FILES
   interfaces/org.freedesktop.Akonadi.Search.xml
   interfaces/org.freedesktop.Akonadi.ControlManager.xml
   interfaces/org.freedesktop.Akonadi.Server.xml
   interfaces/org.freedesktop.Akonadi.SearchQueryIterator.xml
   interfaces/org.freedesktop.Akonadi.TracerNotification.xml
   interfaces/org.freedesktop.Akonadi.SearchQuery.xml
   ${CMAKE_CURRENT_BINARY_DIR}/org.freedesktop.Akonadi.DebugInterface.xml
DESTINATION ${DBUS_INTERFACES_INSTALL_DIR} )

