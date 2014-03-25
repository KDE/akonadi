# Turn exceptions on
kde_enable_exceptions()

set(akonadiagentbase_SRCS
    agentbase.cpp
    agentfactory.cpp
    agentsearchinterface.cpp
    recursivemover.cpp
    resourcebase.cpp
    resourcescheduler.cpp
    resourcesettings.cpp
    transportresourcebase.cpp
)

add_library(KF5AkonadiAgentBase ${akonadiagentbase_SRCS})

generate_export_header(KF5AkonadiAgentBase BASE_NAME akonadiagentbase)

add_library(KF5::AkonadiAgentBase ALIAS KF5AkonadiAgentBase)

target_include_directories(KF5AkonadiAgentBase INTERFACE "$<INSTALL_INTERFACE:${INCLUDE_INSTALL_DIR}/akonadi;${Boost_INCLUDE_DIR}>")
target_include_directories(KF5AkonadiAgentBase PUBLIC "$<BUILD_INTERFACE:${AkonadiAgentBase_SOURCE_DIR}/src;${AkonadiAgentBase_BINARY_DIR}/src>")
target_include_directories(KF5AkonadiAgentBase PUBLIC "$<BUILD_INTERFACE:${AkonadiAgentBase_SOURCE_DIR}/src/agentbase;${AkonadiAgentBase_BINARY_DIR}/src/agentbase>")

target_link_libraries(KF5AkonadiAgentBase
    KF5::KIOCore
    KF5::KDE4Support
)

set_target_properties(KF5AkonadiAgentBase PROPERTIES
    VERSION ${AKONADI_VERSION_STRING}
    SOVERSION ${AKONADI_SOVERSION}
    EXPORT_NAME AkonadiAgentBase
)

# ecm_generate_headers(KSyndication_CamelCase_HEADERS
#   HEADER_NAMES
#   AbstractParser
#   REQUIRED_HEADERS KSyndication_HEADERS
#   PREFIX Syndication
# )


install(TARGETS KF5AkonadiAgentBase EXPORT KF5AkonadiTargets ${INSTALL_TARGETS_DEFAULT_ARGS})

install(FILES
    #${KSyndication_CamelCase_HEADERS}
    DESTINATION ${INCLUDE_INSTALL_DIR}/Akonadi
    COMPONENT Devel
)

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/akonadiagentbase_export.h
    ${AkonadiAgentBase_HEADERS}
    DESTINATION ${INCLUDE_INSTALL_DIR}/akonadi COMPONENT Devel)

ecm_generate_pri_file(BASE_NAME AkonadiAgentBase LIB_NAME KF5AkonadiAgentBase DEPS "KIOCore KDE4Support" FILENAME_VAR PRI_FILENAME)
install(FILES ${PRI_FILENAME} DESTINATION ${ECM_MKSPECS_INSTALL_DIR})
