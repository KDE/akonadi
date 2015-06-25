########### next target ###############

# QTEST_MAIN is using QApplication when QT_GUI_LIB is defined
remove_definitions(-DQT_GUI_LIB)

set(EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR})

include_directories(${CMAKE_CURRENT_BINARY_DIR}
                    ${CMAKE_BINARY_DIR}/src/server
                    ${Akonadi_SOURCE_DIR}/src/server)

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
    dbinitializer.cpp
    ${Akonadi_SOURCE_DIR}/src/server/akonadiserver_debug.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/dbpopulator.cpp
)

add_library(akonadi_unittest_common STATIC ${common_SRCS})
target_link_libraries(akonadi_unittest_common
    KF5AkonadiPrivate
    libakonadiserver
    Qt5::Core
    Qt5::DBus
    Qt5::Test
    Qt5::Sql
    Qt5::Network
)

macro(add_server_test _source)
    set(_test ${_source} ../../src/server/akonadiserver_debug.cpp)
    get_filename_component(_name ${_source} NAME_WE)
    qt5_add_resources(_test dbtest_data/dbtest_data.qrc)
    add_executable(${_name} ${_test})
    add_test(AkonadiServer-${_name} ${_name})
    if (ENABLE_ASAN)
        set_tests_properties(AkonadiServer-${_name} PROPERTIES
                             ENVIRONMENT ASAN_OPTIONS=symbolize=1
        )
    endif()
    target_link_libraries(${_name}
        akonadi_shared
        akonadi_unittest_common
        libakonadiserver
        KF5AkonadiPrivate
        Qt5::Core
        Qt5::DBus
        Qt5::Test
        Qt5::Sql
        Qt5::Network
        ${CMAKE_SHARED_LINKER_FLAGS_ASAN}
    )
endmacro()

add_server_test(dbtypetest.cpp)
add_server_test(dbintrospectortest.cpp)
add_server_test(querybuildertest.cpp)
add_server_test(dbinitializertest.cpp)
add_server_test(dbupdatertest.cpp)
add_server_test(akdbustest.cpp)
add_server_test(akstandarddirstest.cpp)
add_server_test(handlertest.cpp)
add_server_test(dbconfigtest.cpp)
add_server_test(parthelpertest.cpp)
add_server_test(itemretrievertest.cpp)
add_server_test(notificationmanagertest.cpp)
add_server_test(parttypehelpertest.cpp)

if (SQLITE_FOUND) # tests using the fake server need the QSQLITE3 plugin
add_server_test(partstreamertest.cpp)
add_server_test(akappendhandlertest.cpp)
add_server_test(linkhandlertest.cpp)
add_server_test(listhandlertest.cpp)
add_server_test(modifyhandlertest.cpp)
add_server_test(createhandlertest.cpp)
add_server_test(collectionreferencetest.cpp)
add_server_test(searchtest.cpp akonadiprivate)
add_server_test(relationhandlertest.cpp akonadiprivate)
add_server_test(taghandlertest.cpp akonadiprivate)
add_server_test(fetchhandlertest.cpp akonadiprivate)
endif()
