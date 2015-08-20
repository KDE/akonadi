set(akonadiprivate_SRCS
    imapparser.cpp
    imapset.cpp
    instance.cpp
    datastream_p.cpp
    protocol.cpp
    scope.cpp
    tristate.cpp
    xdgbasedirs.cpp
    standarddirs.cpp
)


add_library(KF5AkonadiPrivate ${akonadiprivate_SRCS})
add_library(KF5::AkonadiPrivate ALIAS KF5AkonadiPrivate)
target_link_libraries(KF5AkonadiPrivate
PUBLIC
    Qt5::Core
    Qt5::DBus
)
generate_export_header(KF5AkonadiPrivate BASE_NAME akonadiprivate)

set_target_properties(KF5AkonadiPrivate PROPERTIES
    VERSION ${AKONADI_VERSION_STRING}
    SOVERSION ${AKONADI_SOVERSION}
    EXPORT_NAME AkonadiPrivate
)

install(TARGETS
    KF5AkonadiPrivate
    EXPORT KF5AkonadiServerTargets
    ${KF5_INSTALL_TARGETS_DEFAULT_ARGS}
)

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/akonadiprivate_export.h
    standarddirs_p.h
    imapparser_p.h
    imapset_p.h
    instance_p.h
    protocol_p.h
    protocol_exception_p.h
    xdgbasedirs_p.h
    capabilities_p.h
    scope_p.h
    tristate_p.h
    DESTINATION ${KF5_INCLUDE_INSTALL_DIR}/akonadi/private
)


### Private static library used by unit-tests ####

add_library(akonadiprivate_static STATIC ${akonadiprivate_SRCS})
target_link_libraries(akonadiprivate_static
    Qt5::Core
    Qt5::DBus
)
