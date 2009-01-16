set( EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR} )

set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${KDE4_ENABLE_EXCEPTIONS}" )

include_directories(
    ${CMAKE_SOURCE_DIR}/akonadi
    ${CMAKE_CURRENT_SOURCE_DIR}/../
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}/../
    ${Boost_INCLUDE_DIR}
    ${AKONADI_INCLUDE_DIR}
)

# add testrunner (application for managing a self-contained test
# environment)

add_subdirectory(testrunner)

# add benchmarker
#add_subdirectory(benchmarker)

# convenience macro to add akonadi demo application
macro(add_akonadi_demo _source)
  set(_test ${_source})
  get_filename_component(_name ${_source} NAME_WE)
  kde4_add_executable(${_name} TEST ${_test})
  target_link_libraries(${_name} akonadi-kde akonadi-kmime ${QT_QTCORE_LIBRARY} ${QT_QTGUI_LIBRARY} ${KDE4_KDECORE_LIBS} ${KDE4_KDEUI_LIBS})
endmacro(add_akonadi_demo)


# convenience macro to add akonadi qtestlib unit-tests
macro(add_akonadi_test _source)
  set(_test ${_source})
  get_filename_component(_name ${_source} NAME_WE)
  kde4_add_unit_test(${_name} TESTNAME libakonadi-${_name} ${_test})
  target_link_libraries(${_name} akonadi-kde akonadi-kmime ${QT_QTTEST_LIBRARY} ${QT_QTCORE_LIBRARY} ${QT_QTGUI_LIBRARY} ${KDE4_KDECORE_LIBS} ${AKONADI_COMMON_LIBRARIES})
endmacro(add_akonadi_test)

# demo applications
add_akonadi_demo(itemdumper.cpp)
add_akonadi_demo(collectioncreator.cpp)
add_akonadi_demo(subscriber.cpp)
add_akonadi_demo(headfetcher.cpp)
add_akonadi_demo(agentinstancewidgettest.cpp)
add_akonadi_demo(agenttypewidgettest.cpp)
add_akonadi_demo(pluginloadertest.cpp)
add_akonadi_demo(selftester.cpp)

kde4_add_executable( akonadi-firstrun TEST ../firstrun.cpp firstrunner.cpp )
target_link_libraries( akonadi-firstrun akonadi-kde ${KDE4_KDEUI_LIBS} )

# qtestlib unit tests
add_akonadi_test(imapparsertest.cpp)
add_akonadi_test(imapsettest.cpp)
add_akonadi_test(collectionjobtest.cpp)
add_akonadi_test(collectionpathresolvertest.cpp)
add_akonadi_test(collectionattributetest.cpp)
add_akonadi_test(itemfetchtest.cpp)
add_akonadi_test(itemhydratest.cpp)
add_akonadi_test(itemappendtest.cpp)
add_akonadi_test(itemstoretest.cpp)
add_akonadi_test(monitortest.cpp)
add_akonadi_test(searchjobtest.cpp)
add_akonadi_test(notificationmessagetest.cpp)
add_akonadi_test(itemtest.cpp)
add_akonadi_test(changerecordertest.cpp)
add_akonadi_test(resourcetest.cpp)
add_akonadi_test(subscriptiontest.cpp)
add_akonadi_test(transactiontest.cpp)
add_akonadi_test(itemcopytest.cpp)
add_akonadi_test(collectioncopytest.cpp)
add_akonadi_test(itemserializertest.cpp)
add_akonadi_test(itemsynctest.cpp)
add_akonadi_test(linktest.cpp)
add_akonadi_test(servermanagertest.cpp)
