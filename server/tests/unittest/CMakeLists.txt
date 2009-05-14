########### next target ###############
set( EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR} )

include_directories( ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/../../src)

macro(add_server_test _source)
  set(_test ${_source})
  get_filename_component(_name ${_source} NAME_WE)
  automoc4_add_executable(${_name} ${_source})
  add_test(akonadi-${_name} ${_name})
  target_link_libraries(${_name} akonadiprivate ${QT_QTTEST_LIBRARY})
endmacro(add_server_test)

add_server_test(handlertest.cpp)
if(NOT WIN32)
  add_server_test(imapstreamparsertest.cpp)
endif(NOT WIN32)
add_server_test(scopetest.cpp)
