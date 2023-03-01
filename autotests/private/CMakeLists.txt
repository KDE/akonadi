set(EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR})

macro(add_unit_test _source)
    set(_test ${_source})
    get_filename_component(_name ${_source} NAME_WE)
    ecm_add_test(TEST_NAME ${_name} NAME_PREFIX "AkonadiPrivate-" ${_source})
    if (ENABLE_ASAN)
        set_tests_properties(AkonadiPrivate-${_name} PROPERTIES
                             ENVIRONMENT ASAN_OPTIONS=symbolize=1
        )
    endif()
    target_link_libraries(${_name}
        akonadi_shared
        KPim${KF_MAJOR_VERSION}::AkonadiPrivate
        Qt::Network
        Qt::Widgets
        Qt::Test
        ${CMAKE_EXE_LINKER_FLAGS_ASAN}
    )
endmacro()

add_unit_test(akstandarddirstest.cpp)
add_unit_test(akdbustest.cpp)
add_unit_test(notificationmessagetest.cpp notificationmessagetest.h)
add_unit_test(externalpartstoragetest.cpp)
add_unit_test(protocoltest.cpp protocoltest.h)
add_unit_test(imapparsertest.cpp imapparsertest.h)
add_unit_test(imapsettest.cpp imapsettest.h)
add_unit_test(compressionstreamtest.cpp)
