set(EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR})

macro(add_unit_test _source)
    set(_test ${_source}
        ${Akonadi_BINARY_DIR}/src/akonadicontrol/akonadicontrol_debug.cpp
        ${Akonadi_SOURCE_DIR}/src/akonadicontrol/agenttype.cpp
    )

    get_filename_component(_name ${_source} NAME_WE)
    ecm_add_test(TEST_NAME ${_name} NAME_PREFIX "AkonadiControl-" ${_test})
    add_dependencies(${_name} akonadi_control)
    target_include_directories(${_name} BEFORE PRIVATE $<TARGET_PROPERTY:akonadi_control,INTERFACE_INCLUDE_DIRECTORIES>)
    if (ENABLE_ASAN)
        set_tests_properties(AkonadiControl-${_name} PROPERTIES
                             ENVIRONMENT ASAN_OPTIONS=symbolize=1
        )
    endif()
    target_link_libraries(${_name}
        akonadi_shared
        KF5AkonadiPrivate
        Qt::Test
        KF5::ConfigCore
        ${CMAKE_EXE_LINKER_FLAGS_ASAN}
    )
endmacro()

add_unit_test(agenttypetest.cpp)
