# base install include dir
set(AkonadiAgentBase_INSTALL_INCLUDEDIR "${KDE_INSTALL_INCLUDEDIR}/KPim6/AkonadiAgentBase")

add_library(KPim6AkonadiAgentBase)
add_library(KPim6::AkonadiAgentBase ALIAS KPim6AkonadiAgentBase)

ecm_qt_declare_logging_category(KPim6AkonadiAgentBase HEADER akonadiagentbase_debug.h IDENTIFIER AKONADIAGENTBASE_LOG CATEGORY_NAME org.kde.pim.akonadiagentbase
        DESCRIPTION "akonadi (Akonadi AgentBase Library)"
        OLD_CATEGORY_NAMES akonadiagentbase_log
        EXPORT AKONADI
   )

ecm_generate_headers(AkonadiAgentBase_CC_HEADERS
    HEADER_NAMES
    AccountsIntegration
    AgentBase
    AgentSearchInterface
    PreprocessorBase
    ResourceBase
    ResourceSettings
    TransportResourceBase
    REQUIRED_HEADERS AkonadiAgentBase_HEADERS
    PREFIX Akonadi
)

KCONFIG_ADD_KCFG_FILES(KPim6AkonadiAgentBase resourcebasesettings.kcfgc)

qt_add_dbus_interfaces(akonadiagentbase_SRCS ${Akonadi_SOURCE_DIR}/src/interfaces/org.freedesktop.Akonadi.Tracer.xml)

qt_add_dbus_adaptor(akonadiagentbase_SRCS ${Akonadi_SOURCE_DIR}/src/interfaces/org.freedesktop.Akonadi.Resource.xml
    resourcebase.h Akonadi::ResourceBase resourceadaptor Akonadi__ResourceAdaptor)
qt_add_dbus_adaptor(akonadiagentbase_SRCS ${Akonadi_SOURCE_DIR}/src/interfaces/org.freedesktop.Akonadi.Preprocessor.xml
    preprocessorbase_p.h Akonadi::PreprocessorBasePrivate preprocessoradaptor Akonadi__PreprocessorAdaptor)
qt_add_dbus_adaptor(akonadiagentbase_SRCS ${Akonadi_SOURCE_DIR}/src/interfaces/org.freedesktop.Akonadi.Agent.Status.xml
    agentbase.h Akonadi::AgentBase statusadaptor Akonadi__StatusAdaptor)
qt_add_dbus_adaptor(akonadiagentbase_SRCS ${Akonadi_SOURCE_DIR}/src/interfaces/org.freedesktop.Akonadi.Agent.Control.xml
    agentbase.h Akonadi::AgentBase controladaptor Akonadi__ControlAdaptor)
qt_add_dbus_adaptor(akonadiagentbase_SRCS ../interfaces/org.freedesktop.Akonadi.Resource.Transport.xml
    transportresourcebase_p.h Akonadi::TransportResourceBasePrivate transportadaptor Akonadi__TransportAdaptor)
qt_add_dbus_adaptor(akonadiagentbase_SRCS ../interfaces/org.freedesktop.Akonadi.Agent.Search.xml
                     agentsearchinterface_p.h Akonadi::AgentSearchInterfacePrivate searchadaptor Akonadi__SearchAdaptor)
qt_add_dbus_adaptor(akonadiagentbase_SRCS ../interfaces/org.kde.Akonadi.Accounts.xml
    accountsintegration.h Akonadi::AccountsIntegration accountsadaptor Akonadi__AccountsAdaptor)

target_sources(KPim6AkonadiAgentBase PRIVATE
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
    accountsintegration.h
    agentbase.h
    agentbase_p.h
    agentsearchinterface.h
    preprocessorbase.h
    preprocessorbase_p.h
    recursivemover_p.h
    resourcebase.h
    resourcescheduler_p.h
    resourcesettings.h
    transportresourcebase.h
    ${akonadiagentbase_SRCS}
)

if(COMPILE_WITH_UNITY_CMAKE_SUPPORT)
    set_target_properties(KPim6AkonadiAgentBase PROPERTIES UNITY_BUILD ON)
endif()

generate_export_header(KPim6AkonadiAgentBase BASE_NAME akonadiagentbase)

target_include_directories(KPim6AkonadiAgentBase INTERFACE "$<INSTALL_INTERFACE:${AkonadiAgentBase_INSTALL_INCLUDEDIR}>")

target_link_libraries(KPim6AkonadiAgentBase
PUBLIC
    Qt::DBus
    KPim6::AkonadiCore
    KF6::ConfigCore
    KF6::ConfigGui # for KConfigSkeleton
PRIVATE
    KPim6::AkonadiPrivate
    KF6::I18n
    Qt::Network
    akonadi_shared
)

if(WITH_ACCOUNTS)
    target_link_libraries(KPim6AkonadiAgentBase PRIVATE KAccounts6 Qt::Xml)
    target_include_directories(KPim6AkonadiAgentBase PRIVATE ${ACCOUNTSQT_INCLUDE_DIRS})
endif()

set_target_properties(KPim6AkonadiAgentBase PROPERTIES
    VERSION ${AKONADI_VERSION}
    SOVERSION ${AKONADI_SOVERSION}
    EXPORT_NAME AkonadiAgentBase
)

install(TARGETS
    KPim6AkonadiAgentBase
    EXPORT KPim6AkonadiTargets
    ${KDE_INSTALL_TARGETS_DEFAULT_ARGS}
)

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/akonadiagentbase_export.h
    ${CMAKE_CURRENT_BINARY_DIR}/resourcebasesettings.h
    ${AkonadiAgentBase_HEADERS}
    DESTINATION ${AkonadiAgentBase_INSTALL_INCLUDEDIR}/akonadi
    COMPONENT Devel
)

install(FILES
    ${AkonadiAgentBase_CC_HEADERS}
    DESTINATION ${AkonadiAgentBase_INSTALL_INCLUDEDIR}/Akonadi
    COMPONENT Devel
)

install(FILES
    resourcebase.kcfg
    DESTINATION ${KDE_INSTALL_KCFGDIR}
)

if(BUILD_QCH)
    ecm_add_qch(
        KPim6AkonadiAgent_QCH
        NAME KPim6AkonadiAgent
        BASE_NAME KPim6AkonadiAgent
        VERSION ${PIM_VERSION}
        ORG_DOMAIN org.kde
        SOURCES # using only public headers, to cover only public API
        ${AkonadiAgentBase_HEADERS}
        #MD_MAINPAGE "${CMAKE_SOURCE_DIR}/README.md"
        #IMAGE_DIRS "${CMAKE_SOURCE_DIR}/docs/pics"
        LINK_QCHS
            Qt6Core_QCH
            Qt6Gui_QCH
        INCLUDE_DIRS
            ${CMAKE_CURRENT_BINARY_DIR}
        BLANK_MACROS
            AKONADICORE_EXPORT
        TAGFILE_INSTALL_DESTINATION ${KDE_INSTALL_QTQCHDIR}
        QCH_INSTALL_DESTINATION ${KDE_INSTALL_QTQCHDIR}
        COMPONENT Devel
   )
endif()

## Widgets

set(AkonadiAgentWidgetBase_INSTALL_INCLUDEDIR "${KDE_INSTALL_INCLUDEDIR}/KPim6/AkonadiAgentWidgetBase")

add_library(KPim6AkonadiAgentWidgetBase)
add_library(KPim6::AkonadiAgentWidgetBase ALIAS KPim6AkonadiAgentWidgetBase)

ecm_generate_headers(AkonadiAgentWidgetBase_CC_HEADERS
    HEADER_NAMES
    AgentWidgetBase
    ResourceWidgetBase
    REQUIRED_HEADERS AkonadiAgentWidgetBase_HEADERS
    PREFIX Akonadi
)

target_sources(KPim6AkonadiAgentWidgetBase PRIVATE
    agentwidgetbase.cpp
    resourcewidgetbase.cpp

    agentwidgetbase.h
    resourcewidgetbase.h
)

if(COMPILE_WITH_UNITY_CMAKE_SUPPORT)
    set_target_properties(KPim6AkonadiAgentWidgetBase PROPERTIES UNITY_BUILD ON)
endif()

generate_export_header(KPim6AkonadiAgentWidgetBase BASE_NAME akonadiagentwidgetbase)

target_include_directories(KPim6AkonadiAgentWidgetBase INTERFACE "$<INSTALL_INTERFACE:${AkonadiAgentWidgetBase_INSTALL_INCLUDEDIR}>")

target_link_libraries(KPim6AkonadiAgentWidgetBase
PUBLIC
    Qt::Widgets # for QApplication
    KPim6AkonadiAgentBase
PRIVATE
    KPim6::AkonadiWidgets
    KF6::IconThemes
)

set_target_properties(KPim6AkonadiAgentWidgetBase PROPERTIES
    VERSION ${AKONADI_VERSION}
    SOVERSION ${AKONADI_SOVERSION}
    EXPORT_NAME AkonadiAgentWidgetBase
)

install(TARGETS
    KPim6AkonadiAgentWidgetBase
    EXPORT KPim6AkonadiTargets
    ${KDE_INSTALL_TARGETS_DEFAULT_ARGS}
)

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/akonadiagentwidgetbase_export.h
    ${AkonadiAgentWidgetBase_HEADERS}
    DESTINATION ${AkonadiAgentWidgetBase_INSTALL_INCLUDEDIR}/akonadi
    COMPONENT Devel
)

install(FILES
    ${AkonadiAgentWidgetBase_CC_HEADERS}
    DESTINATION ${AkonadiAgentWidgetBase_INSTALL_INCLUDEDIR}/Akonadi
    COMPONENT Devel
)

if(BUILD_QCH)
    ecm_add_qch(
        KPim6AkonadiWidgetAgent_QCH
        NAME KPim6AkonadiWidgetAgent
        BASE_NAME KPim6AkonadiWidgetAgent
        VERSION ${PIM_VERSION}
        ORG_DOMAIN org.kde
        SOURCES # using only public headers, to cover only public API
        ${AkonadiAgentWidgetBase_HEADERS}
        #MD_MAINPAGE "${CMAKE_SOURCE_DIR}/README.md"
        #IMAGE_DIRS "${CMAKE_SOURCE_DIR}/docs/pics"
        LINK_QCHS
            Qt6Core_QCH
            Qt6Gui_QCH
            Qt6Widgets_QCH
        INCLUDE_DIRS
            ${CMAKE_CURRENT_BINARY_DIR}
        BLANK_MACROS
            AKONADICORE_EXPORT
        TAGFILE_INSTALL_DESTINATION ${KDE_INSTALL_QTQCHDIR}
        QCH_INSTALL_DESTINATION ${KDE_INSTALL_QTQCHDIR}
        COMPONENT Devel
   )
endif()
