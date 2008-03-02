# disable MySQL/Embedded plugin for now since it is unused and causes linker problems on Debian based 64bit systems
# macro_optional_find_package(MySQL)
# macro_log_feature(MYSQL_EMBEDDED_FOUND "mysql-embedded" "MySQL/Embedded Library" "http://www.mysql.org" FALSE "" "Needed for the MySQL/Embedded Akonadi backend. Sqlite will be used instead.")
#
# macro_optional_find_package(OpenSSL)
# macro_log_feature(OPENSSL_FOUND "OpenSSL" "A toolkit implementing the Secure Sockets Layer (SSL v2/v3) and Transport Layer Security (TLS v1) protocols" "http://openssl.org" FALSE "" "Required for building the Akonadi SQL plugin with MySQL embedded.")

find_program( MYSQLD_EXECUTABLE mysqld /usr/sbin /usr/local/sbin /usr/libexec )
macro_log_feature( MYSQLD_EXECUTABLE "MySQL Server" "Database server" "http://www.mysql.com" FALSE ""
                   "Akonadi server requires the mysqld binary" )

include_directories( ${CMAKE_BINARY_DIR}/akonadi )
include_directories( ${CMAKE_SOURCE_DIR}/akonadi )
# needed for akonadi_export.h
include_directories( ${CMAKE_SOURCE_DIR}/akonadi/libakonadi )
include_directories( ${CMAKE_CURRENT_SOURCE_DIR}/src )
include_directories( ${CMAKE_CURRENT_SOURCE_DIR}/src/handler )
include_directories( ${CMAKE_CURRENT_SOURCE_DIR}/tests )
include_directories( ${CMAKE_CURRENT_BINARY_DIR} )
include_directories( ${QT_QTDBUS_INCLUDE_DIR} )

add_subdirectory( akonadictl )
add_subdirectory( control )
add_subdirectory( src )
# if (MYSQL_EMBEDDED_FOUND AND OPENSSL_FOUND)
#   add_subdirectory( sqlplugin )
# endif (MYSQL_EMBEDDED_FOUND AND OPENSSL_FOUND)
add_subdirectory( tests )

set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${KDE4_ENABLE_EXCEPTIONS} -DQT_NO_CAST_FROM_ASCII -DQT_NO_CAST_TO_ASCII" )
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
  src/handler/create.cpp
  src/handler/capability.cpp
  src/handler/delete.cpp
  src/handler/expunge.cpp
  src/handler/fetch.cpp
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
  src/storage/dbinitializer.cpp
  src/storage/dbupdater.cpp
  src/storage/notificationcollector.cpp
  src/storage/query.cpp
  src/storage/querybuilder.cpp
  src/storage/transaction.cpp
  src/tracer.cpp
  src/dbustracer.cpp
  src/filetracer.cpp
  src/notificationmanager.cpp
  src/resourcemanager.cpp
  src/cachecleaner.cpp
  src/xesammanager.cpp
  ../libakonadi/xdgbasedirs.cpp
)

qt4_add_dbus_adaptor( libakonadiprivate_SRCS
  interfaces/org.kde.Akonadi.TracerNotification.xml dbustracer.h Akonadi::DBusTracer
)
qt4_add_dbus_adaptor( libakonadiprivate_SRCS
  interfaces/org.kde.Akonadi.Tracer.xml tracer.h Akonadi::Tracer
)
qt4_add_dbus_adaptor( libakonadiprivate_SRCS
  interfaces/org.kde.Akonadi.NotificationManager.xml notificationmanager.h Akonadi::NotificationManager )

qt4_add_dbus_adaptor( libakonadiprivate_SRCS
  interfaces/org.kde.Akonadi.Server.xml akonadi.h Akonadi::AkonadiServer
)

qt4_generate_dbus_interface( ../libakonadi/resource.h org.kde.Akonadi.Resource.xml )
qt4_add_dbus_interfaces( libakonadiprivate_SRCS
  interfaces/org.kde.Akonadi.AgentManager.xml
  ${CMAKE_CURRENT_BINARY_DIR}/org.kde.Akonadi.Resource.xml
)
qt4_add_dbus_interface2(libakonadiprivate_SRCS
  interfaces/org.freedesktop.xesam.Search.xml
  xesaminterface xesamtypes.h
)

qt4_add_resources( libakonadiprivate_SRCS src/storage/akonadidb.qrc )


kde4_add_library( akonadiprivate SHARED ${libakonadiprivate_SRCS} )
target_link_libraries( akonadiprivate
  ${QT_QTCORE_LIBRARY}
  ${QT_QTNETWORK_LIBRARY}
  ${QT_QTSQL_LIBRARY}
  ${QT_QTDBUS_LIBRARY}
  ${QT_QTXML_LIBRARY}
  ${KDE4_KDECORE_LIBRARY}
  ${KDEWIN32_LIBRARY}
  akonadiprotocol
)
set_target_properties(akonadiprivate PROPERTIES VERSION ${GENERIC_LIB_VERSION} SOVERSION ${GENERIC_LIB_SOVERSION} )
install( TARGETS akonadiprivate DESTINATION ${LIB_INSTALL_DIR} )

set( akonadiserver_bin_SRCS src/main.cpp control/kcrash.cpp )


kde4_add_executable( akonadiserver_bin NOGUI ${akonadiserver_bin_SRCS} )
set_target_properties( akonadiserver_bin PROPERTIES OUTPUT_NAME akonadiserver )
target_link_libraries( akonadiserver_bin akonadiprivate )

install(TARGETS akonadiserver_bin DESTINATION ${BIN_INSTALL_DIR} )
install(FILES
  interfaces/org.kde.Akonadi.AgentManager.xml
  interfaces/org.kde.Akonadi.NotificationManager.xml
  interfaces/org.kde.Akonadi.ProfileManager.xml
  interfaces/org.kde.Akonadi.Tracer.xml
  interfaces/org.kde.Akonadi.TracerNotification.xml
DESTINATION ${DBUS_INTERFACES_INSTALL_DIR})
install(FILES src/storage/mysql-global.conf DESTINATION ${CONFIG_INSTALL_DIR}/akonadi/)
