#
# Convenience macros to add akonadi testrunner unit-tests
#
# Set AKONADI_RUN_ISOLATED_TESTS to false to prevent any isolated Akonadi tests from being run
# Set AKONADI_RUN_MYSQL_ISOLATED_TESTS to false to prevent run the tests against MySQL
# Set AKONADI_RUN_PGSQL_ISOLATED_TESTS to false to prevent run the tests against PostgreSQL
# Set AKONADI_RUN_SQLITE_ISOLATED_TESTS to false to prevent run the tests against SQLite
# Set AKONADI_TESTS_XML to true if you want qtestlib to generate (per backend) XML files with the test results
#
# You still need to provide the test environment, see akonadi/autotests/libs/unittestenv
# copy the unittestenv directory to your unit test directory and update the files
# as necessary

function(add_akonadi_isolated_test)

    function(add_akonadi_isolated_test_impl)
        set(options)
        set(oneValueArgs SOURCE)
        set(multiValueArgs BACKENDS ADDITIONAL_SOURCES LINK_LIBRARIES)
        cmake_parse_arguments(CONFIG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})
        set(_test ${CONFIG_SOURCE})
        get_filename_component(_name ${CONFIG_SOURCE} NAME_WE)
        add_executable(${_name} ${_test} ${CONFIG_ADDITIONAL_SOURCES})
        ecm_mark_as_test(${_name})
        target_link_libraries(${_name}
                              Qt${QT_MAJOR_VERSION}::Test Qt${QT_MAJOR_VERSION}::Gui Qt${QT_MAJOR_VERSION}::Widgets Qt${QT_MAJOR_VERSION}::Network KF5::KIOCore
                              KF5::AkonadiCore KF5::AkonadiPrivate Qt${QT_MAJOR_VERSION}::DBus
                              ${CONFIG_LINK_LIBRARIES}
        )

        if (NOT DEFINED _testrunner)
            if (${PROJECT_NAME} STREQUAL Akonadi AND TARGET akonaditest)
                # If this macro is used in Akonadi itself, just use the target name;
                # CMake will replace it with the path to the executable in the build
                # directory. This will ensure it works even on a clean build,
                # where the executable doesn't exist yet at cmake time.
                set(_testrunner akonaditest)
            else()
                find_program(_testrunner NAMES akonaditest akonaditest.exe)
                if (NOT _testrunner)
                    message(WARNING "Could not locate akonaditest executable, isolated Akonadi tests will fail!")
                endif()
            endif()
        endif()

        # based on kde4_add_unit_test
        set(_executable $<TARGET_FILE:${_name}>)
        if (APPLE)
            set(_executable ${_executable}.app/Contents/MacOS/${_name})
        endif()

        function(_defineTest name backend)
            set(backends ${ARGN})
            if ((NOT DEFINED AKONADI_RUN_${backend}_ISOLATED_TESTS OR AKONADI_RUN_${backend}_ISOLATED_TESTS) AND
                (NOT DEFINED AKONADI_RUN_ISOLATED_TESTS OR AKONADI_RUN_ISOLATED_TESTS))
                LIST(LENGTH "${backends}" backendsLen)
                string(TOLOWER ${backend} lcbackend)
                LIST(FIND "${backends}" ${lcbackend} enableBackend)
                if (${backendsLen} EQUAL 0 OR ${enableBackend} GREATER -1)
                    set(configFile ${CMAKE_CURRENT_SOURCE_DIR}/unittestenv/config.xml)
                    if (AKONADI_TESTS_XML)
                        set(extraOptions -xml -o "${TEST_RESULT_OUTPUT_PATH}/${lcbackend}-${name}.xml")
                    endif()
                    set(_test_name akonadi-${lcbackend}-${name})
                    add_test(NAME ${_test_name}
                             COMMAND ${_testrunner} -c "${configFile}" -b ${lcbackend}
                                     ${_executable} ${extraOptions}
                    )
                    # Taken from ECMAddTests.cmake
                    if (CMAKE_LIBRARY_OUTPUT_DIRECTORY)
                        if(CMAKE_HOST_SYSTEM MATCHES "Windows")
                          set(PATHSEP ";")
                        else() # e.g. Linux
                          set(PATHSEP ":")
                        endif()
                        set(_plugin_path $ENV{QT_PLUGIN_PATH})
                        set(_test_env
                            QT_PLUGIN_PATH=${CMAKE_LIBRARY_OUTPUT_DIRECTORY}${PATHSEP}$ENV{QT_PLUGIN_PATH}
                            LD_LIBRARY_PATH=${CMAKE_LIBRARY_OUTPUT_DIRECTORY}${PATHSEP}$ENV{LD_LIBRARY_PATH}
                        )
                        set_tests_properties(${_test_name} PROPERTIES ENVIRONMENT "${_test_env}")
                      endif()
                endif()
            endif()
        endfunction()

        find_program(MYSQLD_EXECUTABLE mysqld /usr/sbin /usr/local/sbin /usr/libexec /usr/local/libexec /opt/mysql/libexec /usr/mysql/bin)
        if (MYSQLD_EXECUTABLE AND NOT WIN32)
            _defineTest(${_name} "MYSQL" ${CONFIG_BACKENDS})
        endif()

        find_program(POSTGRES_EXECUTABLE postgres)
        if (POSTGRES_EXECUTABLE AND NOT WIN32)
            _defineTest(${_name} "PGSQL" ${CONFIG_BACKENDS})
        endif()

        _defineTest(${_name} "SQLITE" ${CONFIG_BACKENDS})
    endfunction()

    LIST(LENGTH "${ARGN}" argc)
    if (${argc} EQUAL 0)
        add_akonadi_isolated_test_impl(SOURCE ${ARGN})
    else()
        add_akonadi_isolated_test_impl(${ARGN})
    endif()
endfunction()

function(add_akonadi_isolated_test_advanced source additional_sources link_libraries)
    add_akonadi_isolated_test(SOURCE ${source}
                              ADDITIONAL_SOURCES "${additional_sources}"
                              LINK_LIBRARIES "${link_libraries}"
    )
endfunction()

function(kcfg_generate_dbus_interface _kcfg _name)
    find_program(XSLTPROC_EXECUTABLE xsltproc)
    if (NOT XSLTPROC_EXECUTABLE)
        message(FATAL_ERROR "xsltproc executable not found but needed by KCFG_GENERATE_DBUS_INTERFACE()")
    endif()

    # When using this macro inside Akonadi, we need to refer to the file in the
    # repo
    if (Akonadi_SOURCE_DIR)
        set(xsl_path ${Akonadi_SOURCE_DIR}/src/core/kcfg2dbus.xsl)
    else()
        set(xsl_path ${KF5Akonadi_DATA_DIR}/kcfg2dbus.xsl)
    endif()
    file(RELATIVE_PATH xsl_relpath ${CMAKE_CURRENT_BINARY_DIR} ${xsl_path})
    if (IS_ABSOLUTE ${_kcfg})
        file(RELATIVE_PATH kcfg_relpath ${CMAKE_CURRENT_BINARY_DIR} ${_kcfg})
    else()
        file(RELATIVE_PATH kcfg_relpath ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/${_kcfg})
    endif()
    add_custom_command(
        OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${_name}.xml
        COMMAND ${XSLTPROC_EXECUTABLE}
            --output ${_name}.xml
            --stringparam interfaceName ${_name}
            ${xsl_relpath}
            ${kcfg_relpath}
        DEPENDS
            ${xsl_path}
            ${_kcfg}
    )
endfunction()
