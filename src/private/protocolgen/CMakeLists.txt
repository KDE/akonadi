project(protocolgen)

set(protocolgen_SRCS
    main.cpp
    cppgenerator.cpp
    cpphelper.cpp
    nodetree.cpp
    typehelper.cpp
    xmlparser.cpp
)

add_executable(protocolgen ${protocolgen_SRCS})
set_target_properties(protocolgen PROPERTIES MACOSX_BUNDLE FALSE)
target_link_libraries(protocolgen
    Qt5::Core
)
