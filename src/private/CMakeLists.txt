add_subdirectory(protocolgen)

if(NOT XMLLINT_EXECUTABLE)
  message(STATUS "xmllint not found, skipping protocol.xml validation")
else()
  add_test(AkonadiPrivate-protocol-xmllint ${XMLLINT_EXECUTABLE} --noout ${CMAKE_CURRENT_SOURCE_DIR}/protocol.xml)
endif()

add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/protocol_gen.cpp
                          ${CMAKE_CURRENT_BINARY_DIR}/protocol_gen.h
    COMMAND protocolgen ${CMAKE_CURRENT_SOURCE_DIR}/protocol.xml
    DEPENDS protocolgen ${CMAKE_CURRENT_SOURCE_DIR}/protocol.xml
    COMMENT "Generating Protocol implementation"
)

add_custom_target(generate_protocol DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/protocol_gen.cpp)

set(akonadiprivate_SRCS
    imapparser.cpp
    imapset.cpp
    instance.cpp
    compressionstream.cpp
    datastream_p.cpp
    externalpartstorage.cpp
    protocol.cpp
    scope.cpp
    tristate.cpp
    standarddirs.cpp
    dbus.cpp
    imapset_p.h
    instance_p.h
    compressionstream_p.h
    externalpartstorage_p.h
    protocol_p.h
    scope_p.h
    tristate_p.h
    standarddirs_p.h
    dbus_p.h
)

ecm_qt_declare_logging_category(akonadiprivate_SRCS HEADER akonadiprivate_debug.h IDENTIFIER AKONADIPRIVATE_LOG CATEGORY_NAME org.kde.pim.akonadiprivate
        DESCRIPTION "akonadi (Akonadi Private Library)"
        OLD_CATEGORY_NAMES akonadiprivate_log
        EXPORT AKONADI
    )

add_library(KF5AkonadiPrivate SHARED ${akonadiprivate_SRCS})
if (COMPILE_WITH_UNITY_CMAKE_SUPPORT)
    set_target_properties(KF5AkonadiPrivate PROPERTIES UNITY_BUILD ON)
endif()

add_library(KF5::AkonadiPrivate ALIAS KF5AkonadiPrivate)
add_dependencies(KF5AkonadiPrivate generate_protocol)
target_link_libraries(KF5AkonadiPrivate
PUBLIC
    Qt${QT_MAJOR_VERSION}::Core
    Qt${QT_MAJOR_VERSION}::DBus
PRIVATE
    Qt${QT_MAJOR_VERSION}::Network
    LibLZMA::LibLZMA
)
generate_export_header(KF5AkonadiPrivate BASE_NAME akonadiprivate)

set_target_properties(KF5AkonadiPrivate PROPERTIES
    VERSION ${AKONADI_VERSION}
    SOVERSION ${AKONADI_SOVERSION}
    EXPORT_NAME AkonadiPrivate
)

target_include_directories(KF5AkonadiPrivate INTERFACE "$<INSTALL_INTERFACE:${KDE_INSTALL_INCLUDEDIR_KF}/Akonadi>")


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
    ${CMAKE_CURRENT_BINARY_DIR}/protocol_gen.h
    protocol_exception_p.h
    capabilities_p.h
    scope_p.h
    tristate_p.h
    compressionstream_p.h
    DESTINATION ${KDE_INSTALL_INCLUDEDIR_KF}/Akonadi/akonadi/private
)
