set(EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR})

include_directories(${Akonadi_SOURCE_DIR}/src/private
                    ${Akonadi_BINARY_DIR}/src/private)

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
        akonadi_shared
        KF5AkonadiPrivate
        Qt5::Network
        Qt5::Widgets
        Qt5::Test
        ${CMAKE_EXE_LINKER_FLAGS_ASAN}
    )
endmacro()

macro(add_protocol_test _source)
    set(_test ${_source}
        protocoltest.cpp
        ${Akonadi_SOURCE_DIR}/src/private/imapset.cpp
        ${Akonadi_SOURCE_DIR}/src/private/scope.cpp
        ${Akonadi_SOURCE_DIR}/src/private/protocol.cpp
        ${Akonadi_SOURCE_DIR}/src/private/datastream_p.cpp
        ${Akonadi_SOURCE_DIR}/src/private/tristate.cpp
    )
    get_filename_component(_name ${_source} NAME_WE)
    add_executable(${_name} ${_test})
    add_test(AkonadiPrivate-${_name} ${_name})
    if (ENABLE_ASAN)
        set_tests_properties(AkonadiPrivate-${_name} PROPERTIES
                             ENVIRONMENT ASAN_OPTIONS=symbolize=1
        )
    endif()
    target_link_libraries(${_name}
        Qt5::Network
        Qt5::Test
        ${CMAKE_EXE_LINKER_FLAGS_ASAN}
    )
endmacro()

add_unit_test(akstandarddirstest.cpp)
add_unit_test(akdbustest.cpp)
add_unit_test(notificationmessagetest.cpp)
add_unit_test(externalpartstoragetest.cpp)
add_protocol_test(protocoltest.cpp)
