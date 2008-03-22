include_directories(
  ${CMAKE_BINARY_DIR}/akonadi
  ${QT_QTDBUS_INCLUDE_DIR}
  ${SOPRANO_INCLUDE_DIR}
)

set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${KDE4_ENABLE_EXCEPTIONS}" )

########### next target ###############

set( queryserver_SRCS
  main.cpp
  query.cpp
  queryiterator.cpp
  search.cpp
)

qt4_add_dbus_adaptor( queryserver_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/../interfaces/org.kde.Akonadi.Search.xml search.h Search )
qt4_add_dbus_adaptor( queryserver_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/../interfaces/org.kde.Akonadi.SearchQuery.xml query.h Query )
qt4_add_dbus_adaptor( queryserver_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/../interfaces/org.kde.Akonadi.SearchQueryIterator.xml queryiterator.h QueryIterator )

kde4_add_executable( akonadi_queryserver NOGUI ${queryserver_SRCS} )
set_target_properties( akonadi_queryserver PROPERTIES OUTPUT_NAME akonadi_queryserver )

target_link_libraries( akonadi_queryserver ${QT_QTCORE_LIBRARY} ${QT_QTDBUS_LIBRARY} ${NEPOMUK_LIBRARY} )

install( TARGETS akonadi_queryserver DESTINATION ${BIN_INSTALL_DIR} )

configure_file( org.kde.Akonadi.Search.service.cmake ${CMAKE_CURRENT_BINARY_DIR}/org.kde.Akonadi.Search.service )
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/org.kde.Akonadi.Search.service DESTINATION ${CMAKE_INSTALL_PREFIX}/share/dbus-1/services )

########### next target ###############

qt4_add_dbus_interface( queryservertestdbus_SRCS ../interfaces/org.kde.Akonadi.Search.xml searchinterface )
qt4_add_dbus_interface( queryservertestdbus_SRCS ../interfaces/org.kde.Akonadi.SearchQuery.xml searchqueryinterface )
qt4_add_dbus_interface( queryservertestdbus_SRCS ../interfaces/org.kde.Akonadi.SearchQueryIterator.xml searchqueryiteratorinterface )

set( queryservertest_SRCS
  queryservertest.cpp
  ${queryservertestdbus_SRCS}
)

kde4_add_executable( akonadi_queryservertest NOGUI ${queryservertest_SRCS} )
set_target_properties( akonadi_queryservertest PROPERTIES OUTPUT_NAME akonadi_queryservertest )

target_link_libraries( akonadi_queryservertest ${QT_QTCORE_LIBRARY} ${QT_QTDBUS_LIBRARY} ${NEPOMUK_LIBRARY} ${SOPRANO_LIBRARY} )
