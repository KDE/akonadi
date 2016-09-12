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
target_link_libraries(protocolgen
    Qt5::Core
)
