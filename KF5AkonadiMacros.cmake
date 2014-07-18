#
# Convenience macros to add akonadi testrunner unit-tests
#
# Set KDEPIMLIBS_RUN_ISOLATED_TESTS to true to enable running the test
# Set KDEPIMLIBS_RUN_MYSQL_ISOLATED_TESTS to true to run the tests against MySQL
# Set KDEPIMLIBS_RUN_PGSQL_ISOLATED_TESTS to true to run the tests against PostgreSQL
# Set KDEPIMLIBS_RUN_SQLITE_ISOLATED_TESTS to true to run the tests against SQLite
# Set KDEPIMLIBS_TESTS_XML to true if you provided per-test configuration XML files
#
# You still need to provide the test environment, see kdepimlibs/akonadi/tests/unittestenv,
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
  # These are not defined when using within kdepimlibs
  if ( NOT KDEPIMLIBS_AKONADI_CALENDAR_LIBS )
    set( KDEPIMLIBS_AKONADI_CALENDAR_LIBS "KF5::AkonadiCalendar" )
  endif ()
  if ( NOT KDEPIMLIBS_AKONADI_KMIME_LIBS )
    set( KDEPIMLIBS_AKONADI_KMIME_LIBS "KF5::AkonadiMime" )
  endif ()
  if ( NOT KDEPIMLIBS_AKONADI_LIBS )
    set( KDEPIMLIBS_AKONADI_LIBS "akonadi-kde" )
  endif ()
  if ( NOT KDEPIMLIBS_KCALCORE_LIBS )
    set( KDEPIMLIBS_KCALCORE_LIBS "KF5::CalendarCore" )
  endif ()
  if ( NOT KDEPIMLIBS_KCALUTILS_LIBS )
    set( KDEPIMLIBS_KCALUTILS_LIBS "KF5::CalendarUtils" )
  endif ()
  if ( NOT KDEPIMLIBS_KMIME_LIBS )
    set( KDEPIMLIBS_KMIME_LIBS "KF5::KMime" )
  endif ()
  if ( NOT KDEPIMLIBS_KPIMIDENTITIES_LIBS )
    set( KDEPIMLIBS_KPIMIDENTITIES_LIBS "KF5::PimIdentities" )
  endif ()
  if ( NOT KDEPIMLIBS_KPIMUTILS_LIBS )
    set( KDEPIMLIBS_KPIMUTILS_LIBS "KF5::PimUtils" )
  endif ()
  if ( NOT KDEPIMLIBS_MAILTRANSPORT_LIBS )
    set( KDEPIMLIBS_MAILTRANSPORT_LIBS "KF5::MailTransport" )
  endif ()

  set(_test ${_source})
  get_filename_component(_name ${_source} NAME_WE)
  kde4_add_executable(${_name} TEST ${_test} ${_additionalsources})
  target_link_libraries(${_name}
                        ${KDEPIMLIBS_AKONADI_CALENDAR_LIBS}
                        ${KDEPIMLIBS_AKONADI_KMIME_LIBS}
                        ${KDEPIMLIBS_AKONADI_LIBS}
                        ${KDEPIMLIBS_KCALCORE_LIBS}
                        ${KDEPIMLIBS_KCALUTILS_LIBS}
                        ${KDEPIMLIBS_KMIME_LIBS}
                        ${KDEPIMLIBS_KPIMIDENTITIES_LIBS}
                        ${KDEPIMLIBS_KPIMUTILS_LIBS}
                        ${KDEPIMLIBS_MAILTRANSPORT_LIBS}
                        Qt5::Test Qt5::Gui KF5::KIOCore
                        ${_linklibraries})

  # Set the akonaditest path when the macro is used in kdepimlibs
  if(NOT KDEPIMLIBS_BIN_DIR)
    set(_testrunner_BIN_DIR "${_akonaditest_DIR}")
  else()
    set(_testrunner_BIN_DIR "${KDEPIMLIBS_BIN_DIR}")
  endif()
  # based on kde4_add_unit_test
  if (WIN32)
    get_target_property( _loc ${_name} LOCATION )
    set(_executable ${_loc}.bat)
    set(_testrunner ${_testrunner_BIN_DIR}/akonaditest.exe)
  else()
    set(_executable ${EXECUTABLE_OUTPUT_PATH}/${_name})
    set(_testrunner ${_testrunner_BIN_DIR}/akonaditest)
  endif()
  if (UNIX)
    if (APPLE)
      set(_executable ${_executable}.app/Contents/MacOS/${_name})
    else()
      set(_executable ${_executable})
    endif()
    set(_testrunner ${_testrunner})
  endif()

  if ( KDEPIMLIBS_TESTS_XML )
    set( MYSQL_EXTRA_OPTIONS_DB -xml -o ${TEST_RESULT_OUTPUT_PATH}/mysql-db-${_name}.xml )
    set( MYSQL_EXTRA_OPTIONS_FS -xml -o ${TEST_RESULT_OUTPUT_PATH}/mysql-fs-${_name}.xml )
    set( POSTGRESL_EXTRA_OPTIONS_DB -xml -o ${TEST_RESULT_OUTPUT_PATH}/postgresql-db-${_name}.xml )
    set( POSTGRESL_EXTRA_OPTIONS_FS -xml -o ${TEST_RESULT_OUTPUT_PATH}/postgresql-fs-${_name}.xml )
    set( SQLITE_EXTRA_OPTIONS -xml -o ${TEST_RESULT_OUTPUT_PATH}/sqlite-${_name}.xml )
  endif()

  if ( KDEPIMLIBS_RUN_ISOLATED_TESTS )
    if ( KDEPIMLIBS_RUN_MYSQL_ISOLATED_TESTS )
      find_program( MYSQLD_EXECUTABLE mysqld /usr/sbin /usr/local/sbin /usr/libexec /usr/local/libexec /opt/mysql/libexec /usr/mysql/bin )
      if ( MYSQLD_EXECUTABLE )
        add_test( akonadi-mysql-db-${_name} ${_testrunner} -c ${CMAKE_CURRENT_SOURCE_DIR}/unittestenv/config-mysql-db.xml ${_executable}
          ${MYSQL_EXTRA_OPTIONS_DB} )
        add_test( akonadi-mysql-fs-${_name} ${_testrunner} -c ${CMAKE_CURRENT_SOURCE_DIR}/unittestenv/config-mysql-fs.xml ${_executable}
          ${MYSQL_EXTRA_OPTIONS_FS} )
      endif()
    endif()

    if ( KDEPIMLIBS_RUN_PGSQL_ISOLATED_TESTS )
      find_program( POSTGRES_EXECUTABLE postgres )
      if ( POSTGRES_EXECUTABLE )
        add_test( akonadi-postgresql-db-${_name} ${_testrunner} -c ${CMAKE_CURRENT_SOURCE_DIR}/unittestenv/config-postgresql-db.xml ${_executable}
          ${POSTGRESL_EXTRA_OPTIONS_DB} )
        add_test( akonadi-postgresql-fs-${_name} ${_testrunner} -c ${CMAKE_CURRENT_SOURCE_DIR}/unittestenv/config-postgresql-fs.xml ${_executable}
          ${POSTGRESL_EXTRA_OPTIONS_FS} )
      endif()
    endif()

    if ( KDEPIMLIBS_RUN_SQLITE_ISOLATED_TESTS )
      add_test( akonadi-sqlite-${_name} ${_testrunner} -c ${CMAKE_CURRENT_SOURCE_DIR}/unittestenv/config-sqlite-db.xml ${_executable}
        ${SQLITE_EXTRA_OPTIONS} )
    endif()
  endif()
endmacro()


# add_akonadi_isolated_test
#
# Set KDEPIMLIBS_RUN_ISOLATED_TESTS to true to enable running the test
# Set KDEPIMLIBS_RUN_MYSQL_ISOLATED_TESTS to true to run the tests against MySQL
# Set KDEPIMLIBS_RUN_PGSQL_ISOLATED_TESTS to true to run the tests against PostgreSQL
# Set KDEPIMLIBS_RUN_SQLITE_ISOLATED_TESTS to true to run the tests against SQLite
# Set KDEPIMLIBS_TESTS_XML to true to load initial database data from XML files
#
# Arguments:
#       source: a CPP file with the test itself
#
macro(add_akonadi_isolated_test _source)
  add_akonadi_isolated_test_advanced(${_source} "" "")
endmacro()
