
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

#add_akonadi_isolated_test( historytest.cpp )
#add_akonadi_isolated_test( incidencechangertest.cpp )
#add_akonadi_isolated_test( mailclienttest.cpp )
#add_akonadi_isolated_test( calendarbasetest.cpp )
#add_akonadi_isolated_test( fetchjobcalendartest.cpp )
#add_akonadi_isolated_test( etmcalendartest.cpp )
