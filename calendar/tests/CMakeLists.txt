
# set( KDEPIMLIBS_RUN_ISOLATED_TESTS TRUE )

set( PREVIOUS_EXEC_OUTPUT_PATH ../../tests )
set( EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR} )

include(../../AkonadiMacros.cmake)


#add_akonadi_isolated_test( historytest.cpp )
add_akonadi_isolated_test( incidencechangertest.cpp )
#add_akonadi_isolated_test( mailclienttest.cpp )
add_akonadi_isolated_test( calendarbasetest.cpp )
add_akonadi_isolated_test( fetchjobcalendartest.cpp )
add_akonadi_isolated_test( etmcalendartest.cpp )
