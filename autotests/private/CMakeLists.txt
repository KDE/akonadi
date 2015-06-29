set(EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR})

include_directories(${Akonadi_SOURCE_DIR}/src/private)

macro(add_unit_test _source)
    set(_test ${_source})
    get_filename_component(_name ${_source} NAME_WE)
    add_executable(${_name} ${_source})
    add_test(AkonadiPrivate-${_name} ${_name})
    if (ENABLE_ASAN)
        set_tests_properties(AkonadiPrivate-${_name} PROPERTIES
                             ENVIRONMENT ASAN_OPTIONS=symbolize=1
        )
    endif()
    target_link_libraries(${_name}
        KF5AkonadiPrivate
        Qt5::Widgets
        Qt5::Test
        ${CMAKE_EXE_LINKER_FLAGS_ASAN}
    )
endmacro()

add_unit_test(notificationmessagetest.cpp)
