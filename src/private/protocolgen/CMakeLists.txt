project(protocolgen)

add_executable(protocolgen)
target_sources(
    protocolgen
    PRIVATE
        main.cpp
        cppgenerator.cpp
        cpphelper.cpp
        nodetree.cpp
        typehelper.cpp
        xmlparser.cpp
        cppgenerator.h
        cpphelper.h
        nodetree.h
        typehelper.h
        xmlparser.h
)

set_target_properties(
    protocolgen
    PROPERTIES
        MACOSX_BUNDLE
            FALSE
)
target_link_libraries(protocolgen Qt::Core)
