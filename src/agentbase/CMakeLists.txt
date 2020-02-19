set(akonadiagentbase_SRCS
    accountsintegration.cpp
    agentbase.cpp
    agentsearchinterface.cpp
    preprocessorbase.cpp
    preprocessorbase_p.cpp
    recursivemover.cpp
    resourcebase.cpp
    resourcescheduler.cpp
    resourcesettings.cpp
    transportresourcebase.cpp
)

ecm_qt_declare_logging_category(akonadiagentbase_SRCS HEADER akonadiagentbase_debug.h IDENTIFIER AKONADIAGENTBASE_LOG CATEGORY_NAME org.kde.pim.akonadiagentbase)

ecm_generate_headers(AkonadiAgentBase_HEADERS
    HEADER_NAMES
    AccountsIntegration
    AgentBase
    AgentSearchInterface
    PreprocessorBase
    ResourceBase
    ResourceSettings
    TransportResourceBase
    REQUIRED_HEADERS AkonadiAgentBase_HEADERS
)


KCONFIG_ADD_KCFG_FILES(akonadiagentbase_SRCS resourcebasesettings.kcfgc)

qt5_add_dbus_interfaces(akonadiagentbase_SRCS ${Akonadi_SOURCE_DIR}/src/interfaces/org.freedesktop.Akonadi.Tracer.xml )

qt5_add_dbus_adaptor(akonadiagentbase_SRCS ${Akonadi_SOURCE_DIR}/src/interfaces/org.freedesktop.Akonadi.Resource.xml
    resourcebase.h Akonadi::ResourceBase resourceadaptor Akonadi__ResourceAdaptor)
qt5_add_dbus_adaptor(akonadiagentbase_SRCS ${Akonadi_SOURCE_DIR}/src/interfaces/org.freedesktop.Akonadi.Preprocessor.xml
    preprocessorbase_p.h Akonadi::PreprocessorBasePrivate preprocessoradaptor Akonadi__PreprocessorAdaptor)
qt5_add_dbus_adaptor(akonadiagentbase_SRCS ${Akonadi_SOURCE_DIR}/src/interfaces/org.freedesktop.Akonadi.Agent.Status.xml
    agentbase.h Akonadi::AgentBase statusadaptor Akonadi__StatusAdaptor)
qt5_add_dbus_adaptor(akonadiagentbase_SRCS ${Akonadi_SOURCE_DIR}/src/interfaces/org.freedesktop.Akonadi.Agent.Control.xml
    agentbase.h Akonadi::AgentBase controladaptor Akonadi__ControlAdaptor)
qt5_add_dbus_adaptor(akonadiagentbase_SRCS ../interfaces/org.freedesktop.Akonadi.Resource.Transport.xml
    transportresourcebase_p.h Akonadi::TransportResourceBasePrivate transportadaptor Akonadi__TransportAdaptor)
qt5_add_dbus_adaptor(akonadiagentbase_SRCS ../interfaces/org.freedesktop.Akonadi.Agent.Search.xml
                     agentsearchinterface_p.h Akonadi::AgentSearchInterfacePrivate searchadaptor Akonadi__SearchAdaptor )
qt5_add_dbus_adaptor(akonadiagentbase_SRCS ../interfaces/org.kde.Akonadi.Accounts.xml
    accountsintegration.h Akonadi::AccountsIntegration accountsadaptor Akonadi__AccountsAdaptor)

add_library(KF5AkonadiAgentBase ${akonadiagentbase_SRCS})

generate_export_header(KF5AkonadiAgentBase BASE_NAME akonadiagentbase)

add_library(KF5::AkonadiAgentBase ALIAS KF5AkonadiAgentBase)

target_include_directories(KF5AkonadiAgentBase INTERFACE "$<INSTALL_INTERFACE:${KDE_INSTALL_INCLUDEDIR_KF5}/AkonadiAgentBase;${Boost_INCLUDE_DIR}>")

target_link_libraries(KF5AkonadiAgentBase
PUBLIC
    Qt5::DBus
    Qt5::Widgets # for QApplication
    KF5::AkonadiCore
    KF5::ConfigCore
    KF5::ConfigGui # for KConfigSkeleton
PRIVATE
    KF5::AkonadiPrivate
    KF5::AkonadiWidgets
    KF5::I18n
    Qt5::Network
    akonadi_shared
)

if (WITH_ACCOUNTS)
    target_link_libraries(KF5AkonadiAgentBase PRIVATE KAccounts ${ACCOUNTSQT_LIBRARIES})
    target_include_directories(KF5AkonadiAgentBase PRIVATE ${ACCOUNTSQT_INCLUDE_DIRS})
endif()

set_target_properties(KF5AkonadiAgentBase PROPERTIES
    VERSION ${AKONADI_VERSION_STRING}
    SOVERSION ${AKONADI_SOVERSION}
    EXPORT_NAME AkonadiAgentBase
)

ecm_generate_pri_file(BASE_NAME AkonadiAgentBase
    LIB_NAME KF5AkonadiAgentBase
    DEPS "AkonadiCore AkonadiPrivate ConfigCore ConfigGui" FILENAME_VAR PRI_FILENAME
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
    DESTINATION ${KDE_INSTALL_INCLUDEDIR_KF5}/AkonadiAgentBase COMPONENT Devel
)

install(FILES
    resourcebase.kcfg
    DESTINATION ${KDE_INSTALL_KCFGDIR}
)

install(FILES
    ${PRI_FILENAME}
    DESTINATION ${ECM_MKSPECS_INSTALL_DIR}
)
