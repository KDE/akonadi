########### next target ###############

# QTEST_MAIN is using QApplication when QT_GUI_LIB is defined
remove_definitions(-DQT_GUI_LIB)

set(EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR})

include_directories(${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/../../src)

akonadi_generate_schema(${CMAKE_CURRENT_SOURCE_DIR}/dbtest_data/unittest_schema.xml UnitTestSchema unittestschema)

set(AKONADI_DB_DATA ${CMAKE_CURRENT_SOURCE_DIR}/dbtest_data/dbdata.xml)

add_custom_command(
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/dbpopulator.cpp
  COMMAND ${XSLTPROC_EXECUTABLE}
          --output ${CMAKE_CURRENT_BINARY_DIR}/dbpopulator.cpp
          ${CMAKE_CURRENT_SOURCE_DIR}/dbpopulator.xsl
          ${AKONADI_DB_DATA}
  DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/dbpopulator.xsl
          ${AKONADI_DB_DATA}
)

set(common_SRCS
    unittestschema.cpp
    fakeconnection.cpp
    fakedatastore.cpp
    fakeclient.cpp
    fakeakonadiserver.cpp
    fakesearchmanager.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/dbpopulator.cpp
)

add_library(akonadi_unittest_common STATIC ${common_SRCS})
target_link_libraries(akonadi_unittest_common akonadiprivate)

macro(add_server_test _source _libs)
  set(_test ${_source})
  get_filename_component(_name ${_source} NAME_WE)
  qt4_add_resources(_test dbtest_data/dbtest_data.qrc)
  add_executable(${_name} ${_test})
  add_test(akonadi-${_name} ${_name})
  target_link_libraries(${_name} akonadi_shared akonadi_unittest_common ${_libs} ${QT_QTCORE_LIBRARY} ${QT_QTTEST_LIBRARIES} ${QT_QTSQL_LIBRARY} ${QT_QTDBUS_LIBRARY})
  if(AKONADI_STATIC_SQLITE)
    target_link_libraries(${_name} qsqlite3)
  endif()
endmacro()

macro(add_handler_test _source)
  add_server_test(${_source} akonadiprivate)
endmacro()

add_server_test(imapstreamparsertest.cpp akonadiprivate)
add_server_test(scopetest.cpp akonadiprivate)
add_server_test(handlerhelpertest.cpp akonadiprivate)
add_server_test(dbtypetest.cpp akonadiprivate)
add_server_test(dbintrospectortest.cpp akonadiprivate)
add_server_test(querybuildertest.cpp akonadiprivate)
add_server_test(dbinitializertest.cpp akonadiprivate)
add_server_test(dbupdatertest.cpp akonadiprivate)
add_server_test(akdbustest.cpp akonadiprivate)
add_server_test(akstandarddirstest.cpp akonadiprivate)
add_server_test(handlertest.cpp akonadiprivate)
add_server_test(dbconfigtest.cpp akonadiprivate)
add_server_test(parthelpertest.cpp akonadiprivate)
add_server_test(clientcapabilityaggregatortest.cpp akonadiprivate)
add_server_test(fetchscopetest.cpp akonadiprivate)
add_server_test(itemretrievertest.cpp akonadiprivate)
add_server_test(notificationmanagertest.cpp akonadiprivate)
add_server_test(parttypehelpertest.cpp akonadiprivate)

add_server_test(partstreamertest.cpp akonadiprivate)

add_server_test(akappendhandlertest.cpp akonadiprivate)
add_server_test(linkhandlertest.cpp akonadiprivate)
add_server_test(listhandlertest.cpp akonadiprivate)
add_server_test(modifyhandlertest.cpp akonadiprivate)
add_server_test(createhandlertest.cpp akonadiprivate)
add_server_test(collectionreferencetest.cpp akonadiprivate)

add_server_test(searchtest.cpp akonadiprivate)