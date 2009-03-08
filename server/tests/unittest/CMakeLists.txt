########### next target ###############
set( EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR} )

include_directories( ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/../../src)

set(handlertest_SRCS handlertest.cpp teststoragebackend.cpp)

automoc4_add_executable(handlertest ${handlertest_SRCS})
add_test(akonadi-handlertest handlertest)

target_link_libraries(handlertest akonadiprivate ${QT_QTTEST_LIBRARY})

set(imapstreamparsertest_SRCS imapstreamparsertest.cpp)

if(NOT WIN32)
  automoc4_add_executable(imapstreamparsertest ${imapstreamparsertest_SRCS})
  add_test(akonadi-imapstreamparsertest imapstreamparsertest)

  target_link_libraries(imapstreamparsertest akonadiprivate ${QT_QTTEST_LIBRARY})
endif(NOT WIN32)
