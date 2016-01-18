set(QT_REQUIRED_VERSION "5.4.0")
find_package(Qt5Test ${QT_REQUIRED_VERSION} CONFIG REQUIRED)
find_package(Qt5DBus ${QT_REQUIRED_VERSION} CONFIG REQUIRED)
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

kde_enable_exceptions()

include_directories(
  ${CMAKE_CURRENT_BINARY_DIR}
  ${AKONADI_INCLUDE_DIR}
  ${AKONADI_INCLUDE_DIR}/akonadi/private
  ${Boost_INCLUDE_DIR}
)

# add testrunner (application for managing a self-contained test
# environment)

add_subdirectory(testrunner)
add_subdirectory(testresource)
add_subdirectory(testsearchplugin)

# convenience macro to add akonadi qtestlib unit-tests
macro(add_akonadi_test _source)
  set(_test ${_source})
  get_filename_component(_name ${_source} NAME_WE)
  add_executable( ${_name} ${_test} )
  add_test( ${_name} ${_name} )
  ecm_mark_as_test(akonadi-${_name})
  set_tests_properties(${_name} PROPERTIES ENVIRONMENT "QT_HASH_SEED=1;QT_NO_CPU_FEATURE=sse4.2")
  target_link_libraries(${_name} akonaditestfake Qt5::Test KF5::AkonadiPrivate KF5::DBusAddons KF5::I18n)
endmacro()

# convenience macro to add akonadi qtestlib unit-tests
macro(add_akonadi_test_widgets _source)
  set(_test ${_source})
  get_filename_component(_name ${_source} NAME_WE)
  add_executable( ${_name} ${_test} )
  add_test( ${_name} ${_name} )
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

target_link_libraries(akonaditestfake Qt5::DBus KF5::AkonadiCore Qt5::Test Qt5::Widgets KF5::DBusAddons KF5::AkonadiPrivate)

add_executable( akonadi-firstrun ../../src/core/firstrun.cpp firstrunner.cpp )
target_link_libraries( akonadi-firstrun  Qt5::Test Qt5::Core KF5::AkonadiCore KF5::DBusAddons KF5::ConfigCore Qt5::Widgets)

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
add_akonadi_test(entitydisplayattributetest.cpp)
add_akonadi_test(proxymodelstest.cpp)
add_akonadi_test(newmailnotifierattributetest.cpp)
add_akonadi_test(pop3resourceattributetest.cpp)
add_akonadi_test_widgets(actionstatemanagertest.cpp)
add_akonadi_test(tagmodeltest.cpp)

add_akonadi_test(sharedvaluepooltest.cpp)
add_akonadi_test(jobtest.cpp)
add_akonadi_test(tagtest_simple.cpp)
# PORT FROM QJSON add_akonadi_test(searchquerytest.cpp)

# qtestlib tests that need non-exported stuff from 
#add_executable( resourceschedulertest resourceschedulertest.cpp ../src/agentbase/resourcescheduler.cpp )
#add_test( resourceschedulertest resourceschedulertest )
#ecm_mark_as_test(akonadi-resourceschedulertest)
#target_link_libraries(resourceschedulertest  Qt5::Test KF5::AkonadiAgentBase)


# testrunner tests
add_akonadi_isolated_test(testenvironmenttest.cpp)
add_akonadi_isolated_test(autoincrementtest.cpp)
add_akonadi_isolated_test(attributefactorytest.cpp)
add_akonadi_isolated_test(collectionpathresolvertest.cpp)
add_akonadi_isolated_test(collectionattributetest.cpp)
add_akonadi_isolated_test(itemfetchtest.cpp)
add_akonadi_isolated_test(itemappendtest.cpp)
add_akonadi_isolated_test(itemstoretest.cpp)
add_akonadi_isolated_test(itemdeletetest.cpp)
add_akonadi_isolated_test(entitycachetest.cpp)
add_akonadi_isolated_test(monitortest.cpp)
add_akonadi_isolated_test_advanced(monitorfiltertest.cpp "" "KF5::AkonadiPrivate")
#add_akonadi_isolated_test(searchjobtest.cpp)
add_akonadi_isolated_test(changerecordertest.cpp)
add_akonadi_isolated_test(resourcetest.cpp)
add_akonadi_isolated_test(subscriptiontest.cpp)
add_akonadi_isolated_test(transactiontest.cpp)
add_akonadi_isolated_test(itemcopytest.cpp)
add_akonadi_isolated_test(itemmovetest.cpp)
add_akonadi_isolated_test(collectioncopytest.cpp)
add_akonadi_isolated_test(collectionmovetest.cpp)
add_akonadi_isolated_test_advanced(collectionsynctest.cpp "" "KF5::I18n")
add_akonadi_isolated_test(itemsynctest.cpp)
add_akonadi_isolated_test(linktest.cpp)
add_akonadi_isolated_test(cachetest.cpp)

# FIXME: This is very unstable on Jenkins
#add_akonadi_isolated_test(servermanagertest.cpp)

add_akonadi_isolated_test_advanced(tagselectwidgettest.cpp "" "KF5::AkonadiWidgets")


# Having a benchmark is cool if you have any reference to compare against, but this
# benchmark takes over 40 seconds and does not have any real value to us atm. Major
# performance regressions would be spotted by devs anyway, so disabling for now.
#add_akonadi_isolated_test(itembenchmark.cpp)
#add_akonadi_isolated_test(collectioncreator.cpp)

add_akonadi_isolated_test(gidtest.cpp)
add_akonadi_isolated_test(lazypopulationtest.cpp)
add_akonadi_isolated_test(favoriteproxytest.cpp)
add_akonadi_isolated_test_advanced(itemsearchjobtest.cpp testsearchplugin/testsearchplugin.cpp "")
add_akonadi_isolated_test(tagtest.cpp)
add_akonadi_isolated_test(tagsynctest.cpp)
add_akonadi_isolated_test(relationtest.cpp)
add_akonadi_isolated_test(etmpopulationtest.cpp)
