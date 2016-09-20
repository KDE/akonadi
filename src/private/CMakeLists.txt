set(akonadiprivate_SRCS
    imapparser.cpp
    imapset.cpp
    instance.cpp
    datastream_p.cpp
    externalpartstorage.cpp
    protocol.cpp
    scope.cpp
    tristate.cpp
    xdgbasedirs.cpp
    standarddirs.cpp
    dbus.cpp
)

ecm_qt_declare_logging_category(akonadiprivate_SRCS HEADER akonadiprivate_debug.h IDENTIFIER AKONADIPRIVATE_LOG CATEGORY_NAME org.kde.pim.akonadiprivate)

add_library(KF5AkonadiPrivate ${akonadiprivate_SRCS})
add_library(KF5::AkonadiPrivate ALIAS KF5AkonadiPrivate)

target_include_directories(KF5AkonadiPrivate PUBLIC "$<BUILD_INTERFACE:${Akonadi_SOURCE_DIR}/src/private;${Akonadi_BINARY_DIR}/src/private>")


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
    EXPORT KF5AkonadiTargets
    ${KF5_INSTALL_TARGETS_DEFAULT_ARGS}
)

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/akonadiprivate_export.h
    standarddirs_p.h
    dbus_p.h
    imapparser_p.h
    imapset_p.h
    instance_p.h
    externalpartstorage_p.h
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
set_target_properties(akonadiprivate_static PROPERTIES
    COMPILE_FLAGS -DAKONADIPRIVATE_STATIC_DEFINE
)
target_link_libraries(akonadiprivate_static
    Qt5::Core
    Qt5::DBus
)
