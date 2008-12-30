########### next target ###############
# QTEST_MAIN is using QApplication when QT_GUI_LIB is defined
remove_definitions(-DQT_GUI_LIB)

set( EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR} )

include_directories( ${CMAKE_CURRENT_BINARY_DIR} )

set(interpretertest_SRCS interpretertest.cpp ../interpreter.cpp )

automoc4_add_executable(interpretertest ${interpretertest_SRCS})
add_test(akonadi-interpretertest interpretertest)

target_link_libraries(interpretertest ${QT_QTCORE_LIBRARY} ${QT_QTTEST_LIBRARY})


set(querybuildertest_SRCS querybuildertest.cpp)

automoc4_add_executable(querybuildertest ${querybuildertest_SRCS})
add_test(akonadi-querybuildertest querybuildertest)

target_link_libraries(querybuildertest ${QT_QTCORE_LIBRARY} ${QT_QTSQL_LIBRARY} ${QT_QTTEST_LIBRARY})
