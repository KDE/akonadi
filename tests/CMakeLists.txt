set( EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR} )

set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${KDE4_ENABLE_EXCEPTIONS}" )

include_directories(
	${CMAKE_SOURCE_DIR}/akonadi
	${CMAKE_CURRENT_SOURCE_DIR}/../
	${CMAKE_CURRENT_SOURCE_DIR}
        ${Boost_INCLUDE_DIR}
)

# convenience macro to add akonadi demo application
macro(add_akonadi_demo _source)
  set(_test ${_source})
  get_filename_component(_name ${_source} NAME_WE)
  kde4_add_executable(${_name} TEST ${_test})
  target_link_libraries(${_name} akonadi)
endmacro(add_akonadi_demo)


# convenience macro to add akonadi qtestlib unit-tests
macro(add_akonadi_test _source)
  set(_test ${_source})
  get_filename_component(_name ${_source} NAME_WE)
  kde4_add_unit_test(${_name} TESTNAME libakonadi-${_name} ${_test})
  target_link_libraries(${_name} akonadi ${QT_QTTEST_LIBRARY})
endmacro(add_akonadi_test)

# demo applications
add_akonadi_demo(itemdumper.cpp)
add_akonadi_demo(collectioncreator.cpp)
add_akonadi_demo(subscriber.cpp)
#add_akonadi_demo(benchmarker.cpp)
#target_link_libraries(benchmarker akonadi ${QT_QTTEST_LIBRARY})
add_akonadi_demo(headfetcher.cpp)

# qtestlib unit tests
add_akonadi_test(imapparsertest.cpp)
add_akonadi_test(imapsettest.cpp)
add_akonadi_test(datareferencetest.cpp)
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
