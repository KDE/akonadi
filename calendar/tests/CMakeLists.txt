
set( KDEPIMLIBS_RUN_ISOLATED_TESTS TRUE )

set( PREVIOUS_EXEC_OUTPUT_PATH ../../tests )
set( EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR} )

include(../../AkonadiMacros.cmake)

# These are for finding the *_export.h files,
# can be removed when we require CMake 2.8.11
include_directories( ${CMAKE_BINARY_DIR}/kcalcore
                     ${CMAKE_BINARY_DIR}/kmime
                     ${CMAKE_BINARY_DIR}/kcalutils
                     ${CMAKE_BINARY_DIR}/kpimidentities
                     ${CMAKE_BINARY_DIR}/kpimutils
                     ${CMAKE_BINARY_DIR}/akonadi/kmime
                     ${CMAKE_BINARY_DIR}/mailtransport )

add_definitions(-DITIP_DATA_DIR="\\"${CMAKE_CURRENT_SOURCE_DIR}/itip_data\\"" )

set(common_sources unittestbase.cpp helper.cpp ../utils_p.cpp)

#add_akonadi_isolated_test_advanced( historytest.cpp "${common_sources}" "")
#add_akonadi_isolated_test( incidencechangertest.cpp )
#add_akonadi_isolated_test( calendarbasetest.cpp )
#add_akonadi_isolated_test( fetchjobcalendartest.cpp )
#add_akonadi_isolated_test( etmcalendartest.cpp )


if ( PLEASE_TEST_INVITATIONS )
  add_akonadi_isolated_test_advanced( itiphandlertest.cpp "${common_sources}" "")
  add_akonadi_isolated_test( mailclienttest.cpp )
endif()
