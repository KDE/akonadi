include_directories(
  ${SOPRANO_INCLUDE_DIR}
)

set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${_ENABLE_EXCEPTIONS}" )

########### next target ###############

set( queryserver_SRCS
  main.cpp
  query.cpp
  queryiterator.cpp
  search.cpp
)

qt4_add_dbus_adaptor( queryserver_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/../interfaces/org.freedesktop.Akonadi.Search.xml search.h Search )
qt4_add_dbus_adaptor( queryserver_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/../interfaces/org.freedesktop.Akonadi.SearchQuery.xml query.h Query )
qt4_add_dbus_adaptor( queryserver_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/../interfaces/org.freedesktop.Akonadi.SearchQueryIterator.xml queryiterator.h QueryIterator )

add_executable( akonadi_queryserver ${queryserver_SRCS} )
set_target_properties( akonadi_queryserver PROPERTIES OUTPUT_NAME akonadi_queryserver )

target_link_libraries( akonadi_queryserver ${QT_QTCORE_LIBRARY} ${QT_QTDBUS_LIBRARY} ${NEPOMUK_LIBRARY} )

install( TARGETS akonadi_queryserver DESTINATION ${BIN_INSTALL_DIR} )

configure_file( org.freedesktop.Akonadi.Search.service.cmake ${CMAKE_CURRENT_BINARY_DIR}/org.freedesktop.Akonadi.Search.service )
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/org.freedesktop.Akonadi.Search.service DESTINATION ${CMAKE_INSTALL_PREFIX}/share/dbus-1/services )

########### next target ###############

qt4_add_dbus_interface( queryservertestdbus_SRCS ../interfaces/org.freedesktop.Akonadi.Search.xml searchinterface )
qt4_add_dbus_interface( queryservertestdbus_SRCS ../interfaces/org.freedesktop.Akonadi.SearchQuery.xml searchqueryinterface )
qt4_add_dbus_interface( queryservertestdbus_SRCS ../interfaces/org.freedesktop.Akonadi.SearchQueryIterator.xml searchqueryiteratorinterface )

set( queryservertest_SRCS
  queryservertest.cpp
  ${queryservertestdbus_SRCS}
)

add_executable( akonadi_queryservertest ${queryservertest_SRCS} )
set_target_properties( akonadi_queryservertest PROPERTIES OUTPUT_NAME akonadi_queryservertest )

target_link_libraries( akonadi_queryservertest ${QT_QTCORE_LIBRARY} ${QT_QTDBUS_LIBRARY} ${NEPOMUK_LIBRARY} ${SOPRANO_LIBRARY} )
