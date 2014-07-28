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

kde_enable_exceptions()

include_directories(
  ${CMAKE_CURRENT_BINARY_DIR}
  ${AKONADI_INCLUDE_DIR}
  ${AKONADI_INCLUDE_DIR}/akonadi/private
  ${Boost_INCLUDE_DIR}
)

# convenience macro to add akonadi demo application
macro(add_akonadi_demo _source)
  set(_test ${_source})
  get_filename_component(_name ${_source} NAME_WE)
  add_executable(${_name} ${_test})
  target_link_libraries(${_name}  KF5AkonadiCore KF5AkonadiWidgets)
endmacro()

# demo applications
add_akonadi_demo(itemdumper.cpp)
add_akonadi_demo(subscriber.cpp)
add_akonadi_demo(agentinstancewidgettest.cpp)
add_akonadi_demo(agenttypewidgettest.cpp)
##REACTIVATE
#add_akonadi_demo(pluginloadertest.cpp)
##REACTIVATE
#add_akonadi_demo(selftester.cpp)
add_akonadi_demo(collectiondialog.cpp)
