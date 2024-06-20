find_package(Qt6 ${QT_REQUIRED_VERSION} CONFIG REQUIRED Test DBus)

if(${EXECUTABLE_OUTPUT_PATH})
    set(PREVIOUS_EXEC_OUTPUT_PATH ${EXECUTABLE_OUTPUT_PATH})
else()
    set(PREVIOUS_EXEC_OUTPUT_PATH .)
endif()
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR})
set(TEST_RESULT_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR}/testresults)
file(MAKE_DIRECTORY ${TEST_RESULT_OUTPUT_PATH})

option(AKONADI_TESTS_XML "Use XML files for the test results, instead of plain text." FALSE)
option(AKONADI_RUN_SQLITE_ISOLATED_TESTS "Run isolated tests with sqlite3 as backend" TRUE)
option(AKONADI_RUN_MYSQL_ISOLATED_TESTS "Run isolated tests with MySQL as backend" TRUE)
option(AKONADI_RUN_PGSQL_ISOLATED_TESTS "Run isolated tests with PostgreSQL as backend" TRUE)

kde_enable_exceptions()

# convenience macro to add akonadi qtestlib unit-tests
macro(add_akonadi_test _source)
  set(_test ${_source} ${CMAKE_BINARY_DIR}/src/core/akonadicore_debug.cpp)
  get_filename_component(_name ${_source} NAME_WE)
  ecm_add_test(TEST_NAME ${_name} ${_test})
  set_tests_properties(${_name} PROPERTIES ENVIRONMENT "QT_HASH_SEED=0;QT_NO_CPU_FEATURE=sse4.2")
  target_link_libraries(${_name} akonaditestfake Qt::Test KPim6::AkonadiPrivate KF6::I18n)
endmacro()

# convenience macro to add akonadi qtestlib unit-tests
macro(add_akonadi_test_widgets _source)
  set(_test
    ${_source}
    ${CMAKE_BINARY_DIR}/src/widgets/akonadiwidgets_debug.cpp
    ${CMAKE_BINARY_DIR}/src/core/akonadicore_debug.cpp
 )
  get_filename_component(_name ${_source} NAME_WE)
  ecm_add_test(TEST_NAME ${_name} ${_test})
  set_tests_properties(${_name} PROPERTIES ENVIRONMENT "QT_HASH_SEED=0;QT_NO_CPU_FEATURE=sse4.2")
  target_link_libraries(${_name} akonaditestfake Qt::Test KPim6::AkonadiWidgets KPim6::AkonadiPrivate)
endmacro()

include(../KPimAkonadiMacros.cmake)

add_subdirectory(private)
add_subdirectory(server)
add_subdirectory(libs)
add_subdirectory(akonadicontrol)
add_subdirectory(shared)
add_subdirectory(widgets)
