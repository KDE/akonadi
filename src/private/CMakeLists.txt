set(akonadiprivate_SRCS
    imapparser.cpp
    imapset.cpp
    notificationmessage.cpp
    notificationmessagev2.cpp
    notificationmessagev3.cpp
    protocol.cpp
    scope.cpp
    tristate.cpp
    xdgbasedirs.cpp
)

add_library(KF5AkonadiPrivate ${akonadiprivate_SRCS})

generate_export_header(KF5AkonadiPrivate BASE_NAME akonadiprivate)

add_library(KF5::AkonadiPrivate ALIAS KF5AkonadiPrivate)

target_link_libraries(KF5AkonadiPrivate
PRIVATE
    Qt5::Core
    Qt5::DBus
)

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
    imapparser_p.h
    imapset_p.h
    notificationmessage_p.h
    notificationmessagev2_p.h
    notificationmessagev3_p.h
    protocol_p.h
    xdgbasedirs_p.h
    capabilities_p.h
    scope_p.h
    tristate_p.h
    DESTINATION ${KF5_INCLUDE_INSTALL_DIR}/akonadi/private
)
