########### next target ###############
# QTEST_MAIN is using QApplication when QT_GUI_LIB is defined
remove_definitions(-DQT_GUI_LIB)

set( EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR} )

include_directories( ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/../../src)

macro(add_server_test _source _libs)
  set(_test ${_source})
  get_filename_component(_name ${_source} NAME_WE)
  automoc4_add_executable(${_name} ${_source})
  add_test(akonadi-${_name} ${_name})
  target_link_libraries(${_name} ${_libs} ${QT_QTCORE_LIBRARY} ${QT_QTTEST_LIBRARY} ${QT_QTSQL_LIBRARY})
endmacro(add_server_test)

add_server_test(imapstreamparsertest.cpp akonadiprivate)
add_server_test(scopetest.cpp akonadiprivate)
add_server_test(handlerhelpertest.cpp akonadiprivate)
add_server_test(querybuildertest.cpp ${QT_QTSQL_LIBRARY})
#interpreter.cpp is not currently included in the build...
#add_server_test(interpretertest.cpp)
