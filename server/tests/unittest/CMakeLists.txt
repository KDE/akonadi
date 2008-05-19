########### next target ###############
set( EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR} )

include_directories( ${CMAKE_CURRENT_BINARY_DIR} )

set(handlertest_SRCS handlertest.cpp teststoragebackend.cpp )

automoc4(handlertest handlertest_SRCS)
add_executable(handlertest ${handlertest_SRCS})
add_test(akonadi-handlertest handlertest)

target_link_libraries(handlertest akonadiprivate ${QT_QTTEST_LIBRARY})
