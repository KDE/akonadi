set(EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR})

include_directories(${Akonadi_SOURCE_DIR}/src/private)

macro(add_unit_test _source)
    set(_test ${_source})
    get_filename_component(_name ${_source} NAME_WE)
    add_executable(${_name} ${_source})
    add_test(AkonadiPrivate-${_name} ${_name})
    target_link_libraries(${_name}
        KF5AkonadiPrivate
        Qt5::Widgets
        Qt5::Test
    )
endmacro()

add_unit_test(notificationmessagetest.cpp)
add_unit_test(notificationmessagev2test.cpp)
add_unit_test(imapparserbenchmark.cpp)
