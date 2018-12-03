########### next target ###############

# QTEST_MAIN is using QApplication when QT_GUI_LIB is defined
remove_definitions(-DQT_GUI_LIB)

set(EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR})

include_directories(${CMAKE_CURRENT_BINARY_DIR}
                    ${CMAKE_BINARY_DIR}/src/server
                    ${Akonadi_SOURCE_DIR}/src/server)

akonadi_run_xsltproc(
    XSL ${Akonadi_SOURCE_DIR}/src/server/storage/schema.xsl
    XML ${CMAKE_CURRENT_SOURCE_DIR}/dbtest_data/unittest_schema.xml
    BASENAME unittestschema
    CLASSNAME UnitTestSchema
)
akonadi_run_xsltproc(
    XSL ${CMAKE_CURRENT_SOURCE_DIR}/dbpopulator.xsl
    XML ${CMAKE_CURRENT_SOURCE_DIR}/dbtest_data/dbdata.xml
    BASENAME dbpopulator
)

set_property(SOURCE ${CMAKE_CURRENT_BINARY_DIR}/dbpopulator.cpp PROPERTY SKIP_AUTOMOC TRUE)

set(common_SRCS
    unittestschema.cpp
    fakeconnection.cpp
    fakedatastore.cpp
    fakeclient.cpp
    fakeakonadiserver.cpp
    fakesearchmanager.cpp
    fakeitemretrievalmanager.cpp
    dbinitializer.cpp
    inspectablenotificationcollector.cpp
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
    set(_test ${_source} ../../src/server/akonadiserver_debug.cpp ../../src/server/akonadiserver_search_debug.cpp)
    get_filename_component(_name ${_source} NAME_WE)
    qt5_add_resources(_test dbtest_data/dbtest_data.qrc)
    add_executable(${_name} ${_test})
    add_test(NAME AkonadiServer-${_name} COMMAND ${_name})
    if (ENABLE_ASAN)
        set_tests_properties(AkonadiServer-${_name} PROPERTIES
                             ENVIRONMENT ASAN_OPTIONS=symbolize=1
        )
    endif()
    set_tests_properties(AkonadiServer-${_name} PROPERTIES
                         ENVIRONMENT "QT_HASH_SEED=0;QT_NO_CPU_FEATURE=sse4.2"
    )
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
add_server_test(handlertest.cpp)
add_server_test(dbconfigtest.cpp)
add_server_test(parthelpertest.cpp)
add_server_test(itemretrievertest.cpp)
add_server_test(notificationmanagertest.cpp)
add_server_test(parttypehelpertest.cpp)
add_server_test(collectionstatisticstest.cpp)

if (SQLITE_FOUND) # tests using the fake server need the QSQLITE3 plugin
add_server_test(partstreamertest.cpp)
add_server_test(itemcreatehandlertest.cpp)
add_server_test(itemlinkhandlertest.cpp)
add_server_test(itemmovehandlertest.cpp)
add_server_test(collectioncreatehandlertest.cpp)
add_server_test(collectionfetchhandlertest.cpp)
add_server_test(collectionmodifyhandlertest.cpp)
add_server_test(collectionreferencetest.cpp)
add_server_test(searchtest.cpp akonadiprivate)
add_server_test(relationhandlertest.cpp akonadiprivate)
add_server_test(taghandlertest.cpp akonadiprivate)
add_server_test(fetchhandlertest.cpp akonadiprivate)
endif()
