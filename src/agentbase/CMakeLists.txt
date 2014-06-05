# Turn exceptions on
kde_enable_exceptions()

set(akonadiagentbase_SRCS
    agentbase.cpp
    agentfactory.cpp
    agentsearchinterface.cpp
    preprocessorbase.cpp
    preprocessorbase_p.cpp
    recursivemover.cpp
    resourcebase.cpp
    resourcescheduler.cpp
    resourcesettings.cpp
    transportresourcebase.cpp
)

ecm_generate_headers(AkonadiAgentBase_HEADERS
    HEADER_NAMES
    AgentBase
    AgentFactory
    AgentSearchInterface
    ResourceBase
    ResourceSettings
    TransportResourceBase
    REQUIRED_HEADERS AkonadiAgentBase_HEADERS
)


KCONFIG_ADD_KCFG_FILES(akonadiagentbase_SRCS resourcebasesettings.kcfgc)

qt5_add_dbus_interfaces(akonadiagentbase_SRCS ${AKONADI_DBUS_INTERFACES_DIR}/org.freedesktop.Akonadi.Tracer.xml )

qt5_add_dbus_adaptor(akonadiagentbase_SRCS ${AKONADI_DBUS_INTERFACES_DIR}/org.freedesktop.Akonadi.Resource.xml
    resourcebase.h Akonadi::ResourceBase resourceadaptor Akonadi__ResourceAdaptor)
qt5_add_dbus_adaptor(akonadiagentbase_SRCS ${AKONADI_DBUS_INTERFACES_DIR}/org.freedesktop.Akonadi.Preprocessor.xml
    preprocessorbase_p.h Akonadi::PreprocessorBasePrivate preprocessoradaptor Akonadi__PreprocessorAdaptor)
qt5_add_dbus_adaptor(akonadiagentbase_SRCS ${AKONADI_DBUS_INTERFACES_DIR}/org.freedesktop.Akonadi.Agent.Status.xml
    agentbase.h Akonadi::AgentBase statusadaptor Akonadi__StatusAdaptor)
qt5_add_dbus_adaptor(akonadiagentbase_SRCS ${AKONADI_DBUS_INTERFACES_DIR}/org.freedesktop.Akonadi.Agent.Control.xml
    agentbase.h Akonadi::AgentBase controladaptor Akonadi__ControlAdaptor)
qt5_add_dbus_adaptor(akonadiagentbase_SRCS ../interfaces/org.freedesktop.Akonadi.Resource.Transport.xml
    transportresourcebase_p.h Akonadi::TransportResourceBasePrivate transportadaptor Akonadi__TransportAdaptor)
qt5_add_dbus_adaptor(akonadiagentbase_SRCS ../interfaces/org.freedesktop.Akonadi.Agent.Search.xml
                     agentsearchinterface_p.h Akonadi::AgentSearchInterfacePrivate searchadaptor Akonadi__SearchAdaptor )

add_library(KF5AkonadiAgentBase ${akonadiagentbase_SRCS})

generate_export_header(KF5AkonadiAgentBase BASE_NAME akonadiagentbase)

add_library(KF5::AkonadiAgentBase ALIAS KF5AkonadiAgentBase)

target_include_directories(KF5AkonadiAgentBase INTERFACE "$<INSTALL_INTERFACE:${KF5_INCLUDE_INSTALL_DIR}/AkonadiAgentBase;${Boost_INCLUDE_DIR}>")

target_link_libraries(KF5AkonadiAgentBase
PUBLIC
    KF5::AkonadiCore
    KF5::ConfigCore
    KF5::KDELibs4Support
PRIVATE
    KF5::Solid
)

set_target_properties(KF5AkonadiAgentBase PROPERTIES
    VERSION ${AKONADILIBRARIES_VERSION_STRING}
    SOVERSION ${AKONADILIBRARIES_SOVERSION}
    EXPORT_NAME AkonadiAgentBase
)

ecm_generate_pri_file(BASE_NAME AkonadiAgentBase
    LIB_NAME KF5AkonadiAgentBase
    DEPS "KIOCore KDELibs4Support" FILENAME_VAR PRI_FILENAME
)

install(TARGETS
    KF5AkonadiAgentBase
    EXPORT KF5AkonadiTargets
    ${KF5_INSTALL_TARGETS_DEFAULT_ARGS}
)

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/akonadiagentbase_export.h
    ${CMAKE_CURRENT_BINARY_DIR}/resourcebasesettings.h
    ${AkonadiAgentBase_HEADERS}
    DESTINATION ${KF5_INCLUDE_INSTALL_DIR}/AkonadiAgentBase COMPONENT Devel
)

install(FILES
    resourcebase.kcfg
    DESTINATION ${KCFG_INSTALL_DIR}
)

install(FILES
    ${PRI_FILENAME}
    DESTINATION ${ECM_MKSPECS_INSTALL_DIR}
)
