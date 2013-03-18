set(EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/..)

macro(add_unit_test _source)
  set(_test ${_source})
  get_filename_component(_name ${_source} NAME_WE)
  add_executable(${_name} ${_source})
  add_test(akonadi-${_name} ${_name})
  target_link_libraries(${_name} akonadiprotocolinternals ${QT_QTGUI_LIBRARY} ${QT_QTTEST_LIBRARIES})
endmacro()

add_unit_test(notificationmessagetest.cpp)
add_unit_test(notificationmessagev2test.cpp)
add_unit_test(imapparserbenchmark.cpp)
