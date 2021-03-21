add_subdirectory(protocolgen)

include_directories(${LIBLZMA_INCLUDE_DIRS})

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
)

set(akonadiprivate_LIBS
PUBLIC
    Qt5::Core
    Qt5::DBus
PRIVATE
    ${LIBLZMA_LIBRARIES}
)
if (WIN32)
    set(akonadiprivate_LIBS
    ${akonadiprivate_LIBS}
    PRIVATE
        Qt5::Network
    )
endif()

ecm_qt_declare_logging_category(akonadiprivate_SRCS HEADER akonadiprivate_debug.h IDENTIFIER AKONADIPRIVATE_LOG CATEGORY_NAME org.kde.pim.akonadiprivate
        DESCRIPTION "akonadi (Akonadi Private Library)"
        OLD_CATEGORY_NAMES akonadiprivate_log
        EXPORT AKONADI
    )

macro(update_include_directories _target)
    target_include_directories(${_target} PUBLIC "$<BUILD_INTERFACE:${Akonadi_SOURCE_DIR}/src/private;${Akonadi_BINARY_DIR}/src/private;${Akonadi_BINARY_DIR}/src>")
    target_include_directories(${_target} PRIVATE "${Akonadi_SOURCE_DIR}/src/shared")
    target_include_directories(${_target} PUBLIC "$<BUILD_INTERFACE:${Qt5Core_INCLUDE_DIRS};${Qt5DBus_INCLUDE_DIRS}>")
endmacro()

if (WIN32)
    # MSVC does not like when the same object files are reused for shared and
    # static linking, so in this case we build all sources twice to make it happy
    set(akonadiprivate_buildsources ${akonadiprivate_SRCS})
else()
    add_library(akonadiprivate_obj OBJECT ${akonadiprivate_SRCS})
    update_include_directories(akonadiprivate_obj)
    set_target_properties(akonadiprivate_obj PROPERTIES POSITION_INDEPENDENT_CODE 1)
    add_dependencies(akonadiprivate_obj generate_protocol)
    set(akonadiprivate_buildsources $<TARGET_OBJECTS:akonadiprivate_obj>)
endif()

add_library(KF5AkonadiPrivate SHARED ${akonadiprivate_buildsources})

add_library(KF5::AkonadiPrivate ALIAS KF5AkonadiPrivate)
if (WIN32)
    add_dependencies(KF5AkonadiPrivate generate_protocol)
    update_include_directories(KF5AkonadiPrivate)
endif()
target_link_libraries(KF5AkonadiPrivate ${akonadiprivate_LIBS})
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
    ${CMAKE_CURRENT_BINARY_DIR}/protocol_gen.h
    protocol_exception_p.h
    capabilities_p.h
    scope_p.h
    tristate_p.h
    compressionstream_p.h
    DESTINATION ${KDE_INSTALL_INCLUDEDIR_KF5}/akonadi/private
)


### Private static library used by unit-tests ####

add_library(akonadiprivate_static STATIC ${akonadiprivate_buildsources})
if (WIN32)
    add_dependencies(akonadiprivate_static generate_protocol)
    update_include_directories(akonadiprivate_static)
endif()
set_target_properties(akonadiprivate_static PROPERTIES
    COMPILE_FLAGS -DAKONADIPRIVATE_STATIC_DEFINE
)
target_link_libraries(akonadiprivate_static ${akonadiprivate_LIBS})
