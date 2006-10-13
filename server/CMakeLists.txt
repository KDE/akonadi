macro_optional_find_package(OpenSSL)

include_directories( ${CMAKE_CURRENT_SOURCE_DIR}/include )
include_directories( ${CMAKE_CURRENT_SOURCE_DIR}/src/searchproviders )
include_directories( ${CMAKE_CURRENT_SOURCE_DIR}/src )
include_directories( ${CMAKE_CURRENT_SOURCE_DIR}/src/handler )
include_directories( ${CMAKE_CURRENT_SOURCE_DIR}/tests )
include_directories( ${QT_QTDBUS_INCLUDE_DIR} )

add_subdirectory( control )
add_subdirectory( src )
if (MYSQL_EMBEDDED_FOUND AND OPENSSL_FOUND)
  add_subdirectory( sqlplugin )
endif (MYSQL_EMBEDDED_FOUND AND OPENSSL_FOUND)
add_subdirectory( tests )

set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fexceptions -DQT_NO_CAST_FROM_ASCII -DQT_NO_CAST_TO_ASCII" )
set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DAKONADI_USE_SQLITE" )
#set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DAKONADI_USE_POSTGRES" )
#set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DAKONADI_USE_MYSQL_EMBEDDED" )
#set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DAKONADI_USE_MYSQL" )

########### next target ###############
set(libakonadiprivate_SRCS
  src/akonadi.cpp
  src/akonadiconnection.cpp
  src/handler.cpp
  src/handlerhelper.cpp
  src/response.cpp
  src/handler/append.cpp
  src/handler/create.cpp
  src/handler/capability.cpp
  src/handler/delete.cpp
  src/handler/expunge.cpp
  src/handler/fetch.cpp
  src/handler/fetchquery.cpp
  src/handler/list.cpp
  src/handler/login.cpp
  src/handler/logout.cpp
  src/handler/modify.cpp
  src/handler/noop.cpp
  src/handler/rename.cpp
  src/handler/search.cpp
  src/handler/searchhelper.cpp
  src/handler/searchpersistent.cpp
  src/handler/select.cpp
  src/handler/status.cpp
  src/handler/store.cpp
  src/handler/storequery.cpp
  src/handler/transaction.cpp
  src/handler/uid.cpp
#  src/persistentsearch.cpp
#  src/persistentsearchmanager.cpp
#  src/searchprovider.cpp
  src/searchprovidermanager.cpp
#  src/searchproviders/emailsearchprovider.cpp
  src/storage/entity.cpp
  src/storage/datastore.cpp
  src/storage/dbinitializer.cpp
  src/storage/debug.cpp
  src/storage/notificationcollector.cpp
  src/storage/transaction.cpp
  src/collection.cpp
  src/tracer.cpp
  src/dbustracer.cpp
  src/dbusthread.cpp
  src/filetracer.cpp
  src/notificationmanager.cpp
  src/resourcemanager.cpp
  src/imapparser.cpp
)

qt4_add_dbus_adaptor( libakonadiprivate_SRCS
  interfaces/org.kde.Akonadi.NotificationManager.xml notificationmanager.h Akonadi::NotificationManager
)
qt4_add_dbus_adaptor( libakonadiprivate_SRCS
  interfaces/org.kde.Akonadi.TracerNotification.xml dbustracer.h Akonadi::DBusTracer
)
qt4_add_dbus_adaptor( libakonadiprivate_SRCS
  interfaces/org.kde.Akonadi.Tracer.xml tracer.h Akonadi::Tracer
)

qt4_add_dbus_interfaces( libakonadiprivate_SRCS interfaces/org.kde.Akonadi.AgentManager.xml ../resources/include/org.kde.Akonadi.Resource.xml)

qt4_add_resources( libakonadiprivate_SRCS src/storage/akonadidb.qrc )

kde4_add_library( akonadiprivate SHARED ${libakonadiprivate_SRCS} )
target_link_libraries( akonadiprivate ${QT_QTCORE_LIBRARY} ${QT_QTNETWORK_LIBRARIES}
                                      ${QT_QTSQL_LIBRARIES} ${QT_QTDBUS_LIBRARIES} ${QT_QTXML_LIBRARIES})
install( TARGETS akonadiprivate DESTINATION ${LIB_INSTALL_DIR} )

set( akonadiserver_bin_SRCS src/main.cpp )

kde4_automoc( ${libakonadiprivate_SRCS} akonadiserver_bin ${akonadiserver_bin_SRCS} )

kde4_add_executable( akonadiserver_bin ${akonadiserver_bin_SRCS} )
set_target_properties( akonadiserver_bin PROPERTIES OUTPUT_NAME akonadiserver )
target_link_libraries( akonadiserver_bin akonadiprivate )

install(TARGETS akonadiserver_bin DESTINATION ${BIN_INSTALL_DIR} )
