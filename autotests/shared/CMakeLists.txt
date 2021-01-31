set(EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR})

include_directories(${Akonadi_SOURCE_DIR}/src/shared)

macro(add_unit_test _source)
    set(_test ${_source})
    get_filename_component(_name ${_source} NAME_WE)
    ecm_add_test(TEST_NAME ${_name} NAME_PREFIX "AkonadiShared-" ${_source})
    if (ENABLE_ASAN)
        set_tests_properties(AkonadiShared-${_name} PROPERTIES
                             ENVIRONMENT ASAN_OPTIONS=symbolize=1
        )
    endif()
    target_link_libraries(${_name}
        akonadi_shared
        Qt::Test
        ${CMAKE_EXE_LINKER_FLAGS_ASAN}
    )
endmacro()

add_unit_test(akrangestest.cpp)
add_unit_test(akscopeguardtest.cpp)
