# convenience macro to add akonadi testrunner unit-tests
macro(add_akonadi_isolated_test _source)
  set(_test ${_source})
  get_filename_component(_name ${_source} NAME_WE)
  kde4_add_executable(${_name} TEST ${_test})
  target_link_libraries(${_name}
                        akonadi-calendar
                        akonadi-kde
                        akonadi-kmime
                        mailtransport
                        kcalcore
                        kcalutils
                        kmime
                        kpimidentities
                        kpimutils
                        ${QT_QTTEST_LIBRARY} ${QT_QTCORE_LIBRARY} ${QT_QTGUI_LIBRARY} ${KDE4_KDECORE_LIBS} ${KDE4_KIO_LIBS} ${AKONADI_COMMON_LIBRARIES})

  # based on kde4_add_unit_test
  if (WIN32)
    get_target_property( _loc ${_name} LOCATION )
    set(_executable ${_loc}.bat)
    set(_testrunner ${PREVIOUS_EXEC_OUTPUT_PATH}/akonaditest.exe.bat)
  else()
    set(_executable ${EXECUTABLE_OUTPUT_PATH}/${_name})
    set(_testrunner ${PREVIOUS_EXEC_OUTPUT_PATH}/akonaditest)
  endif()
  if (UNIX)
    if (APPLE)
      set(_executable ${_executable}.app/Contents/MacOS/${_name})
    else()
      set(_executable ${_executable}.shell)
    endif()
    set(_testrunner ${_testrunner}.shell)
  endif()

  if ( KDEPIMLIBS_TESTS_XML )
    set( MYSQL_EXTRA_OPTIONS_DB -xml -o ${TEST_RESULT_OUTPUT_PATH}/mysql-db-${_name}.xml )
    set( MYSQL_EXTRA_OPTIONS_FS -xml -o ${TEST_RESULT_OUTPUT_PATH}/mysql-fs-${_name}.xml )
    set( POSTGRESL_EXTRA_OPTIONS_DB -xml -o ${TEST_RESULT_OUTPUT_PATH}/postgresql-db-${_name}.xml )
    set( POSTGRESL_EXTRA_OPTIONS_FS -xml -o ${TEST_RESULT_OUTPUT_PATH}/postgresql-fs-${_name}.xml )
    set( SQLITE_EXTRA_OPTIONS -xml -o ${TEST_RESULT_OUTPUT_PATH}/sqlite-${_name}.xml )
  endif()

  if ( KDEPIMLIBS_RUN_ISOLATED_TESTS )
    find_program( MYSQLD_EXECUTABLE mysqld /usr/sbin /usr/local/sbin /usr/libexec /usr/local/libexec /opt/mysql/libexec /usr/mysql/bin )
    if ( MYSQLD_EXECUTABLE )
      add_test( akonadi-mysql-db-${_name} ${_testrunner} -c ${CMAKE_CURRENT_SOURCE_DIR}/unittestenv/config-mysql-db.xml ${_executable}
        ${MYSQL_EXTRA_OPTIONS_DB} )
      add_test( akonadi-mysql-fs-${_name} ${_testrunner} -c ${CMAKE_CURRENT_SOURCE_DIR}/unittestenv/config-mysql-fs.xml ${_executable}
        ${MYSQL_EXTRA_OPTIONS_FS} )
    endif()

    find_program( POSTGRES_EXECUTABLE postgres )
    if ( POSTGRES_EXECUTABLE )
      add_test( akonadi-postgresql-db-${_name} ${_testrunner} -c ${CMAKE_CURRENT_SOURCE_DIR}/unittestenv/config-postgresql-db.xml ${_executable}
        ${POSTGRESL_EXTRA_OPTIONS_DB} )
      add_test( akonadi-postgresql-fs-${_name} ${_testrunner} -c ${CMAKE_CURRENT_SOURCE_DIR}/unittestenv/config-postgresql-fs.xml ${_executable}
        ${POSTGRESL_EXTRA_OPTIONS_FS} )
    endif()

    if ( KDEPIMLIBS_TESTS_SQLITE )
      add_test( akonadi-sqlite-${_name} ${_testrunner} -c ${CMAKE_CURRENT_SOURCE_DIR}/unittestenv/config-sqlite-db.xml ${_executable}
        ${SQLITE_EXTRA_OPTIONS} )
    endif()
  endif()
endmacro()
