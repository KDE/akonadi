#
# Convenience macros to add akonadi testrunner unit-tests
#
# Set AKONADI_RUN_ISOLATED_TESTS to true to enable running the test
# Set AKONADI_RUN_MYSQL_ISOLATED_TESTS to true to run the tests against MySQL
# Set AKONADI_RUN_PGSQL_ISOLATED_TESTS to true to run the tests against PostgreSQL
# Set AKONADI_RUN_SQLITE_ISOLATED_TESTS to true to run the tests against SQLite
# Set AKONADI_TESTS_XML to true if you provided per-test configuration XML files
#
# You still need to provide the test environment, see akonadi/autotests/libs/unittestenv
# copy the unittestenv directory to your unit test directory and update the files
# as necessary

# add_akonadi_isolated_test_advanced
#
# Arguments:
#       source: a CPP file with the test itself
#       additionalSources: additional CPP files needed to build the test
#       linklibraries: additional libraries to link the test executable against
#
macro(add_akonadi_isolated_test_advanced _source _additionalsources _linklibraries)
  set(_test ${_source})
  get_filename_component(_name ${_source} NAME_WE)
  add_executable( ${_name} ${_test} ${_additionalsources})
  ecm_mark_as_test(${_name})
  target_link_libraries(${_name}
                        Qt5::Test Qt5::Gui Qt5::Widgets KF5::KIOCore KF5::AkonadiCore KF5::DBusAddons
                        ${_linklibraries})

  # Set the akonaditest path when the macro is used in Akonadi
  find_program(_testrunner
               NAMES akonaditest akonaditest.exe
               PATHS ${CMAKE_CURRENT_BINARY_DIR} ${_akonaditest_DIR} ENV PATH)
  if (_testrunner-NOTFOUND)
    message(WARNING "Could not locate akonaditest executable, isolated Akonadi tests will fail!")
  endif()

  # based on kde4_add_unit_test
  if (WIN32)
    get_target_property( _loc ${_name} LOCATION )
    set(_executable ${_loc}.bat)
  else()
    set(_executable ${EXECUTABLE_OUTPUT_PATH}/${_name})
  endif()
  if (UNIX)
    if (APPLE)
      set(_executable ${_executable}.app/Contents/MacOS/${_name})
    else()
      set(_executable ${_executable})
    endif()
  endif()

  if ( KDEPIMLIBS_TESTS_XML OR AKONADI_TESTS_XML )
    set( MYSQL_EXTRA_OPTIONS_DB -xml -o ${TEST_RESULT_OUTPUT_PATH}/mysql-db-${_name}.xml )
    set( MYSQL_EXTRA_OPTIONS_FS -xml -o ${TEST_RESULT_OUTPUT_PATH}/mysql-fs-${_name}.xml )
    set( POSTGRESL_EXTRA_OPTIONS_DB -xml -o ${TEST_RESULT_OUTPUT_PATH}/postgresql-db-${_name}.xml )
    set( POSTGRESL_EXTRA_OPTIONS_FS -xml -o ${TEST_RESULT_OUTPUT_PATH}/postgresql-fs-${_name}.xml )
    set( SQLITE_EXTRA_OPTIONS -xml -o ${TEST_RESULT_OUTPUT_PATH}/sqlite-${_name}.xml )
  endif()

  if ( KDEPIMLIBS_RUN_MYSQL_ISOLATED_TESTS OR AKONADI_RUN_MYSQL_ISOLATED_TESTS )
    find_program( MYSQLD_EXECUTABLE mysqld /usr/sbin /usr/local/sbin /usr/libexec /usr/local/libexec /opt/mysql/libexec /usr/mysql/bin )
    if ( MYSQLD_EXECUTABLE )
      add_test( akonadi-mysql-db-${_name} ${_testrunner} -c ${CMAKE_CURRENT_SOURCE_DIR}/unittestenv/config-mysql-db.xml ${_executable}
        ${MYSQL_EXTRA_OPTIONS_DB} )
      add_test( akonadi-mysql-fs-${_name} ${_testrunner} -c ${CMAKE_CURRENT_SOURCE_DIR}/unittestenv/config-mysql-fs.xml ${_executable}
        ${MYSQL_EXTRA_OPTIONS_FS} )
    endif()
  endif()

  if ( KDEPIMLIBS_RUN_PGSQL_ISOLATED_TESTS OR AKONADI_RUN_PGSQL_ISOLATED_TESTS )
    find_program( POSTGRES_EXECUTABLE postgres )
    if ( POSTGRES_EXECUTABLE )
    add_test( akonadi-postgresql-db-${_name} ${_testrunner} -c ${CMAKE_CURRENT_SOURCE_DIR}/unittestenv/config-postgresql-db.xml ${_executable}
        ${POSTGRESL_EXTRA_OPTIONS_DB} )
    add_test( akonadi-postgresql-fs-${_name} ${_testrunner} -c ${CMAKE_CURRENT_SOURCE_DIR}/unittestenv/config-postgresql-fs.xml ${_executable}
        ${POSTGRESL_EXTRA_OPTIONS_FS} )
    endif()
  endif()

  # Always have SQLITE tests
  add_test( akonadi-sqlite-${_name} ${_testrunner} -c ${CMAKE_CURRENT_SOURCE_DIR}/unittestenv/config-sqlite-db.xml ${_executable}
    ${SQLITE_EXTRA_OPTIONS} )
endmacro()


# add_akonadi_isolated_test
#
# Set AKONADI_RUN_ISOLATED_TESTS to true to enable running the test
# Set AKONADI_RUN_MYSQL_ISOLATED_TESTS to true to run the tests against MySQL
# Set AKONADI_RUN_PGSQL_ISOLATED_TESTS to true to run the tests against PostgreSQL
# Set AKONADI_RUN_SQLITE_ISOLATED_TESTS to true to run the tests against SQLite
# Set AKONADI_TESTS_XML to true to load initial database data from XML files
#
# Arguments:
#       source: a CPP file with the test itself
#
macro(add_akonadi_isolated_test _source)
  add_akonadi_isolated_test_advanced(${_source} "" "")
endmacro()
