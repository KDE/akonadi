########### next target ###############
set( EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR} )

include_directories( ${CMAKE_CURRENT_BINARY_DIR} )

set(interpretertest_SRCS interpretertest.cpp ../interpreter.cpp )

kde4_add_unit_test(interpretertest TESTNAME akonadi-interpretertest ${interpretertest_SRCS})

target_link_libraries(interpretertest ${QT_QTCORE_LIBRARY} ${QT_QTTEST_LIBRARY})


kde4_add_unit_test(querybuildertest TESTNAME akonadi-querybuildertest querybuildertest.cpp)
target_link_libraries(querybuildertest ${QT_QTCORE_LIBRARY} ${QT_QTSQL_LIBRARY} ${QT_QTTEST_LIBRARY})
