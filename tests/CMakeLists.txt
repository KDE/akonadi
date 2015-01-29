if(${EXECUTABLE_OUTPUT_PATH})
    set( PREVIOUS_EXEC_OUTPUT_PATH ${EXECUTABLE_OUTPUT_PATH} )
else()
    set( PREVIOUS_EXEC_OUTPUT_PATH . )
endif()
set( EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR} )
set( TEST_RESULT_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR}/testresults )
file(MAKE_DIRECTORY ${TEST_RESULT_OUTPUT_PATH})

option(KDEPIMLIBS_TESTS_XML "Use XML files for the test results, instead of plain text." FALSE)
option(KDEPIMLIBS_RUN_SQLITE_ISOLATED_TESTS "Run isolated tests with sqlite3 as backend" TRUE)
option(KDEPIMLIBS_RUN_MYSQL_ISOLATED_TESTS "Run isolated tests with MySql as backend" FALSE)
option(KDEPIMLIBS_RUN_PGSQL_ISOLATED_TESTS "Run isolated tests with PostgreSQL as backend" FALSE)

set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${KDE4_ENABLE_EXCEPTIONS}" )

include_directories(
  ${CMAKE_CURRENT_BINARY_DIR}
  ${AKONADI_INCLUDE_DIR}
  ${AKONADI_INCLUDE_DIR}/akonadi/private
  ${Boost_INCLUDE_DIR}
  ${QJSON_INCLUDE_DIR}
  ${CMAKE_SOURCE_DIR}/akonadi
  ${CMAKE_BINARY_DIR}/akonadi
  ${CMAKE_BINARY_DIR}/akonadi/kmime
  ${CMAKE_BINARY_DIR}/kmime
)

# add testrunner (application for managing a self-contained test
# environment)

add_subdirectory(etm_test_app)
add_subdirectory(testrunner)
add_subdirectory(testresource)
add_subdirectory(testsearchplugin)

# add benchmarker
add_subdirectory(benchmarker)

# convenience macro to add akonadi demo application
macro(add_akonadi_demo _source)
  set(_test ${_source})
  get_filename_component(_name ${_source} NAME_WE)
  kde4_add_executable(${_name} TEST ${_test})
  target_link_libraries(${_name} akonadi-kde akonadi-kmime ${QT_QTCORE_LIBRARY} ${QT_QTGUI_LIBRARY} ${KDE4_KDECORE_LIBS} ${KDE4_KDEUI_LIBS})
endmacro()

# convenience macro to add akonadi qtestlib unit-tests
macro(add_akonadi_test _source)
  set(_test ${_source})
  get_filename_component(_name ${_source} NAME_WE)
  kde4_add_unit_test(${_name} TESTNAME akonadi-${_name} ${_test})
  target_link_libraries(${_name} akonadi-kde akonadi-kmime akonaditestfake ${QT_QTTEST_LIBRARY} ${QT_QTCORE_LIBRARY} ${QT_QTNETWORK_LIBRARY} ${QT_QTGUI_LIBRARY} ${KDE4_KDECORE_LIBS} ${AKONADI_COMMON_LIBRARIES} ${QJSON_LIBRARIES})
endmacro()

include(../AkonadiMacros.cmake)

# akonadi test fake library
set(akonaditestfake_xml ${AKONADI_DBUS_INTERFACES_DIR}/org.freedesktop.Akonadi.NotificationManager.xml)
set_source_files_properties(${akonaditestfake_xml} PROPERTIES INCLUDE "notificationmessagev3_p.h")
qt4_add_dbus_interface( akonaditestfake_srcs ${akonaditestfake_xml} notificationmanagerinterface )

set(akonaditestfake_xml ${AKONADI_DBUS_INTERFACES_DIR}/org.freedesktop.Akonadi.NotificationSource.xml)
set_source_files_properties(${akonaditestfake_xml} PROPERTIES INCLUDE "notificationmessagev3_p.h")
qt4_add_dbus_interface( akonaditestfake_srcs ${akonaditestfake_xml} notificationsourceinterface )

add_library(akonaditestfake STATIC
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

# demo applications
add_akonadi_demo(itemdumper.cpp)
add_akonadi_demo(subscriber.cpp)
add_akonadi_demo(headfetcher.cpp)
add_akonadi_demo(agentinstancewidgettest.cpp)
add_akonadi_demo(agenttypewidgettest.cpp)
add_akonadi_demo(pluginloadertest.cpp)
add_akonadi_demo(selftester.cpp)
add_akonadi_demo(collectiondialog.cpp)

kde4_add_executable( akonadi-firstrun TEST ../firstrun.cpp firstrunner.cpp )
target_link_libraries( akonadi-firstrun akonadi-kde ${KDE4_KDEUI_LIBS} )

# qtestlib unit tests
add_akonadi_test(imapparsertest.cpp)
add_akonadi_test(imapsettest.cpp)
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
add_akonadi_test(actionstatemanagertest.cpp)
add_akonadi_test(sharedvaluepooltest.cpp)
add_akonadi_test(jobtest.cpp)
add_akonadi_test(searchquerytest.cpp)

# qtestlib tests that need non-exported stuff from akonadi-kde
kde4_add_unit_test(resourceschedulertest TESTNAME akonadi-resourceschedulertest resourceschedulertest.cpp ../resourcescheduler.cpp)
target_link_libraries(resourceschedulertest akonadi-kde ${QT_QTTEST_LIBRARY} ${QT_QTCORE_LIBRARY} ${QT_QTGUI_LIBRARY} ${KDE4_KDECORE_LIBS} ${AKONADI_COMMON_LIBRARIES})


# testrunner tests
add_akonadi_isolated_test(testenvironmenttest.cpp)
add_akonadi_isolated_test(autoincrementtest.cpp)
add_akonadi_isolated_test(attributefactorytest.cpp)
add_akonadi_isolated_test(collectionjobtest.cpp)
add_akonadi_isolated_test(collectionpathresolvertest.cpp)
add_akonadi_isolated_test(collectionattributetest.cpp)
add_akonadi_isolated_test(itemfetchtest.cpp)
add_akonadi_isolated_test(itemappendtest.cpp)
add_akonadi_isolated_test(itemstoretest.cpp)
add_akonadi_isolated_test(itemdeletetest.cpp)
add_akonadi_isolated_test(entitycachetest.cpp)
add_akonadi_isolated_test(monitortest.cpp)
add_akonadi_isolated_test(monitorfiltertest.cpp)
add_akonadi_isolated_test(searchjobtest.cpp)
add_akonadi_isolated_test(changerecordertest.cpp)
add_akonadi_isolated_test(resourcetest.cpp)
add_akonadi_isolated_test(subscriptiontest.cpp)
add_akonadi_isolated_test(transactiontest.cpp)
add_akonadi_isolated_test(itemcopytest.cpp)
add_akonadi_isolated_test(itemmovetest.cpp)
add_akonadi_isolated_test(collectioncopytest.cpp)
add_akonadi_isolated_test(collectionmovetest.cpp)
add_akonadi_isolated_test(collectionsynctest.cpp)
add_akonadi_isolated_test(itemsynctest.cpp)
add_akonadi_isolated_test(linktest.cpp)
add_akonadi_isolated_test(cachetest.cpp)
# starting/stopping the server in the testenvironment doesn't seem to work, 
# and the test is not very critical (we'll notice if we can't start the server), so we disable it for now.
#add_akonadi_isolated_test(servermanagertest.cpp)
add_akonadi_isolated_test(collectioncreator.cpp)


# Having a benchmark is cool if you have any reference to compare against, but this
# benchmark takes over 40 seconds and does not have any real value to us atm. Major
# performance regressions would be spotted by devs anyway, so disabling for now.
#add_akonadi_isolated_test(itembenchmark.cpp)
add_akonadi_isolated_test(gidtest.cpp)
add_akonadi_isolated_test(lazypopulationtest.cpp)
add_akonadi_isolated_test(favoriteproxytest.cpp)
add_akonadi_isolated_test_advanced(itemsearchjobtest.cpp testsearchplugin/testsearchplugin.cpp "")
add_akonadi_isolated_test(tagtest.cpp)
add_akonadi_isolated_test(etmpopulationtest.cpp)
