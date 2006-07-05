include_directories( ${CMAKE_CURRENT_SOURCE_DIR}/include )
include_directories( ${CMAKE_CURRENT_SOURCE_DIR}/src/searchproviders )
include_directories( ${CMAKE_CURRENT_SOURCE_DIR}/src )
include_directories( ${CMAKE_CURRENT_SOURCE_DIR}/src/handler )
include_directories( ${CMAKE_CURRENT_SOURCE_DIR}/test )
include_directories( ${QT_QTDBUS_INCLUDE_DIR} )

add_subdirectory( src )
add_subdirectory( control )
add_subdirectory( debugger )

set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fexceptions" )

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
  src/handler/expunge.cpp
  src/handler/fetch.cpp
  src/handler/fetchquery.cpp
  src/handler/list.cpp
  src/handler/login.cpp
  src/handler/logout.cpp
  src/handler/noop.cpp
  src/handler/search.cpp
  src/handler/searchhelper.cpp
  src/handler/searchpersistent.cpp
  src/handler/select.cpp
  src/handler/status.cpp
  src/handler/store.cpp
  src/handler/storequery.cpp
  src/handler/uid.cpp
  test/teststoragebackend.cpp
  src/persistentsearch.cpp
  src/persistentsearchmanager.cpp
  src/searchprovider.cpp
  src/searchprovidermanager.cpp
  src/searchproviders/emailsearchprovider.cpp
  src/storage/entity.cpp
  src/storage/datastore.cpp
  src/storage/debug.cpp
  src/collection.cpp
  src/tracer.cpp
  src/dbustracer.cpp
  src/filetracer.cpp
  src/notificationmanager.cpp
)

qt4_add_dbus_adaptor( libakonadiprivate_SRCS
  src/org.kde.Akonadi.NotificationManager.xml notificationmanager.h Akonadi::NotificationManager
)
qt4_add_dbus_adaptor( libakonadiprivate_SRCS
  src/org.kde.Akonadi.Tracer.xml dbustracer.h Akonadi::DBusTracer
)


qt4_add_dbus_interfaces( libakonadiprivate_SRCS control/org.kde.Akonadi.AgentManager.xml )

set(akonadiserver_bin_SRCS ${libakonadiprivate_SRCS}
    src/main.cpp
)

kde4_automoc(${libakonadiprivate_SRCS} ${akonadiserver_bin_SRCS})

kde4_add_executable(akonadi_bin ${akonadiserver_bin_SRCS})
set_target_properties(akonadi_bin PROPERTIES OUTPUT_NAME akonadi)
target_link_libraries(akonadi_bin ${QT_QTCORE_LIBRARIES}  ${QT_QTNETWORK_LIBRARIES} ${QT_QTSQL_LIBRARIES} ${QT_QTDBUS_LIBRARIES})

install(TARGETS akonadi_bin DESTINATION ${BIN_INSTALL_DIR} )


