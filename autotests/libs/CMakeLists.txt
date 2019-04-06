set(QT_REQUIRED_VERSION "5.9.0")
find_package(Qt5 ${QT_REQUIRED_VERSION} CONFIG REQUIRED Test DBus)
include(ECMAddTests)


if(${EXECUTABLE_OUTPUT_PATH})
    set( PREVIOUS_EXEC_OUTPUT_PATH ${EXECUTABLE_OUTPUT_PATH} )
else()
    set( PREVIOUS_EXEC_OUTPUT_PATH . )
endif()
set( EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR} )
set( TEST_RESULT_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR}/testresults )
file(MAKE_DIRECTORY ${TEST_RESULT_OUTPUT_PATH})

option(AKONADI_TESTS_XML "Use XML files for the test results, instead of plain text." FALSE)
option(AKONADI_RUN_SQLITE_ISOLATED_TESTS "Run isolated tests with sqlite3 as backend" TRUE)
option(AKONADI_RUN_MYSQL_ISOLATED_TESTS "Run isolated tests with MySQL as backend" TRUE)
option(AKONADI_RUN_PGSQL_ISOLATED_TESTS "Run isolated tests with PostgreSQL as backend" TRUE)

kde_enable_exceptions()

include_directories(
  ${Boost_INCLUDE_DIR}
)

# convenience macro to add akonadi qtestlib unit-tests
macro(add_akonadi_test _source)
  set(_test ${_source} ${CMAKE_BINARY_DIR}/src/core/akonadicore_debug.cpp)
  get_filename_component(_name ${_source} NAME_WE)
  add_executable( ${_name} ${_test} )
  add_test(NAME ${_name} COMMAND ${_name} )
  ecm_mark_as_test(akonadi-${_name})
  set_tests_properties(${_name} PROPERTIES ENVIRONMENT "QT_HASH_SEED=1;QT_NO_CPU_FEATURE=sse4.2")
  target_link_libraries(${_name} akonaditestfake Qt5::Test KF5::AkonadiPrivate KF5::DBusAddons KF5::I18n)
endmacro()

# convenience macro to add akonadi qtestlib unit-tests
macro(add_akonadi_test_widgets _source)
  set(_test
    ${_source}
    ${CMAKE_BINARY_DIR}/src/widgets/akonadiwidgets_debug.cpp
    ${CMAKE_BINARY_DIR}/src/core/akonadicore_debug.cpp
  )
  get_filename_component(_name ${_source} NAME_WE)
  add_executable( ${_name} ${_test} )
  add_test(NAME ${_name} COMMAND ${_name} )
  ecm_mark_as_test(akonadi-${_name})
  set_tests_properties(${_name} PROPERTIES ENVIRONMENT "QT_HASH_SEED=1;QT_NO_CPU_FEATURE=sse4.2")
  target_link_libraries(${_name} akonaditestfake Qt5::Test KF5::AkonadiWidgets KF5::AkonadiPrivate KF5::DBusAddons)
endmacro()


include(../../KF5AkonadiMacros.cmake)

# akonadi test fake library
set(akonaditestfake_xml ${Akonadi_SOURCE_DIR}/src/interfaces/org.freedesktop.Akonadi.NotificationSource.xml)
set_source_files_properties(${akonaditestfake_xml} PROPERTIES INCLUDE "protocol_p.h")
qt5_add_dbus_interface( akonaditestfake_srcs ${akonaditestfake_xml} notificationsourceinterface )

add_library(akonaditestfake SHARED
  ${akonaditestfake_srcs}
  fakeakonadiservercommand.cpp
  fakesession.cpp
  fakemonitor.cpp
  fakeserverdata.cpp
  modelspy.cpp
  fakeentitycache.cpp
  inspectablemonitor.cpp
  inspectablechangerecorder.cpp
)

generate_export_header(akonaditestfake BASE_NAME akonaditestfake)

target_link_libraries(akonaditestfake
        Qt5::DBus
        KF5::AkonadiCore
        Qt5::Test
        Qt5::Widgets
        Qt5::Network
        KF5::DBusAddons
        KF5::AkonadiPrivate
        akonadi_shared
)

add_executable(akonadi-firstrun
    ../../src/core/firstrun.cpp
    firstrunner.cpp
    ${CMAKE_BINARY_DIR}/src/core/akonadicore_debug.cpp
)
target_link_libraries( akonadi-firstrun  Qt5::Test Qt5::Core KF5::AkonadiCore KF5::AkonadiPrivate KF5::DBusAddons KF5::ConfigCore Qt5::Widgets)

# qtestlib unit tests
add_akonadi_test(imapparsertest.cpp)
# It need KIMAP add_akonadi_test(imapsettest.cpp)
add_akonadi_test(itemhydratest.cpp)
add_akonadi_test(itemtest.cpp)
add_akonadi_test(itemserializertest.cpp)
add_akonadi_test(mimetypecheckertest.cpp)
add_akonadi_test(protocolhelpertest.cpp)
add_akonadi_test(entitytreemodeltest.cpp)
add_akonadi_test(monitornotificationtest.cpp)
add_akonadi_test(collectionutilstest.cpp)
add_akonadi_test(collectioncolorattributetest.cpp)
add_akonadi_test(entitydisplayattributetest.cpp)
add_akonadi_test(proxymodelstest.cpp)
add_akonadi_test(newmailnotifierattributetest.cpp)
add_akonadi_test(pop3resourceattributetest.cpp)
add_akonadi_test_widgets(actionstatemanagertest.cpp)
add_akonadi_test_widgets(conflictresolvedialogtest.cpp)
add_akonadi_test(tagmodeltest.cpp)
add_akonadi_test(statisticsproxymodeltest.cpp)

add_akonadi_test(sharedvaluepooltest.cpp)
add_akonadi_test(jobtest.cpp)
add_akonadi_test(tagtest_simple.cpp)
add_akonadi_test(cachepolicytest.cpp)

# PORT FROM QJSON add_akonadi_test(searchquerytest.cpp)

# qtestlib tests that need non-exported stuff from 
#add_executable( resourceschedulertest resourceschedulertest.cpp ../src/agentbase/resourcescheduler.cpp )
#add_test( resourceschedulertest resourceschedulertest )
#ecm_mark_as_test(akonadi-resourceschedulertest)
#target_link_libraries(resourceschedulertest  Qt5::Test KF5::AkonadiAgentBase)


# testrunner tests
add_akonadi_isolated_test(SOURCE testenvironmenttest.cpp)
add_akonadi_isolated_test(SOURCE autoincrementtest.cpp)
add_akonadi_isolated_test(SOURCE attributefactorytest.cpp)
add_akonadi_isolated_test(SOURCE collectionpathresolvertest.cpp)
add_akonadi_isolated_test(SOURCE collectionattributetest.cpp)
add_akonadi_isolated_test(SOURCE itemfetchtest.cpp)
add_akonadi_isolated_test(SOURCE itemappendtest.cpp)
add_akonadi_isolated_test(SOURCE itemstoretest.cpp)
add_akonadi_isolated_test(SOURCE itemdeletetest.cpp)
add_akonadi_isolated_test(SOURCE entitycachetest.cpp)
add_akonadi_isolated_test(SOURCE monitortest.cpp)
#add_akonadi_isolated_test_advanced(monitorfiltertest.cpp "" "KF5::AkonadiPrivate")
# FIXME: this is constantly failling due to broken search: re-enable once the new search code is merged
#add_akonadi_isolated_test(SOURCE searchjobtest.cpp)
add_akonadi_isolated_test(SOURCE changerecordertest.cpp)
add_akonadi_isolated_test(SOURCE resourcetest.cpp)
add_akonadi_isolated_test(SOURCE subscriptiontest.cpp)
add_akonadi_isolated_test(SOURCE transactiontest.cpp)
add_akonadi_isolated_test(SOURCE itemcopytest.cpp)
add_akonadi_isolated_test(SOURCE itemmovetest.cpp)
add_akonadi_isolated_test(SOURCE collectioncreatetest.cpp)
add_akonadi_isolated_test(SOURCE collectioncopytest.cpp)
add_akonadi_isolated_test(SOURCE collectionmovetest.cpp)
add_akonadi_isolated_test(
    SOURCE  collectionsynctest.cpp
    ADDITIONAL_SOURCES ${CMAKE_BINARY_DIR}/src/core/akonadicore_debug.cpp
    LINK_LIBRARIES KF5::I18n
)
add_akonadi_isolated_test(SOURCE itemsynctest.cpp)
add_akonadi_isolated_test(SOURCE linktest.cpp)
add_akonadi_isolated_test(SOURCE cachetest.cpp)
add_akonadi_isolated_test(SOURCE collectionjobtest.cpp)
add_akonadi_isolated_test(SOURCE collectionmodifytest.cpp)
add_akonadi_isolated_test(SOURCE referencetest.cpp)

# FIXME: This is very unstable on Jenkins
#add_akonadi_isolated_test(servermanagertest.cpp)

add_akonadi_isolated_test(
    SOURCE tagselectwidgettest.cpp
    LINK_LIBRARIES KF5::AkonadiWidgets
)


# Having a benchmark is cool if you have any reference to compare against, but this
# benchmark takes over 40 seconds and does not have any real value to us atm. Major
# performance regressions would be spotted by devs anyway, so disabling for now.
#add_akonadi_isolated_test(itembenchmark.cpp)
#add_akonadi_isolated_test(collectioncreator.cpp)

add_akonadi_isolated_test(SOURCE gidtest.cpp)
add_akonadi_isolated_test(SOURCE lazypopulationtest.cpp)
add_akonadi_isolated_test(SOURCE favoriteproxytest.cpp)
# FIXME: this is constantly failing due to broken search: re-enable once the new search code is merged
#add_akonadi_isolated_test(
#    SOURCE itemsearchjobtest.cpp
#    ADDITIONAL_SOURCES testsearchplugin/testsearchplugin.cpp)
add_akonadi_isolated_test(SOURCE tagtest.cpp ADDITIONAL_SOURCES ${CMAKE_BINARY_DIR}/src/core/akonadicore_debug.cpp)
add_akonadi_isolated_test(SOURCE tagsynctest.cpp)
add_akonadi_isolated_test(SOURCE relationtest.cpp)
add_akonadi_isolated_test(SOURCE etmpopulationtest.cpp)
