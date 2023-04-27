
# base install include dir
set(AkonadiQml_INSTALL_INCLUDEDIR "${KDE_INSTALL_INCLUDEDIR}/KPim${KF_MAJOR_VERSION}/AkonadiQml")

set(akonadiqml_SRCS
    agentqmlconfigurationbase.cpp

    agentqmlconfigurationbase.h
)

ecm_generate_headers(AkonadiQml_CC_HEADERS
    HEADER_NAMES
    AgentQmlConfigurationBase
    REQUIRED_HEADERS AkonadiQml_HEADERS
    PREFIX AkonadiQml
)

ecm_qt_declare_logging_category(akonadiqml_SRCS HEADER akonadiqml_debug.h IDENTIFIER AKONADIQML_LOG CATEGORY_NAME org.kde.pim.akonadiqml
    DESCRIPTION "akonadi (Akonadi QML Library)"
    OLD_CATEGORY_NAMES log_akonadiqml
    EXPORT AKONADI
)

add_library(KPim${KF_MAJOR_VERSION}AkonadiQml ${akonadiqml_SRCS})
generate_export_header(KPim${KF_MAJOR_VERSION}AkonadiQml BASE_NAME akonadiqml)

add_library(KPim${KF_MAJOR_VERSION}::AkonadiQml ALIAS KPim${KF_MAJOR_VERSION}AkonadiQml)
target_include_directories(KPim${KF_MAJOR_VERSION}AkonadiQml INTERFACE "$<INSTALL_INTERFACE:${AkonadiQml_INSTALL_INCLUDEDIR}>")
target_include_directories(KPim${KF_MAJOR_VERSION}AkonadiQml PUBLIC "$<BUILD_INTERFACE:${Akonadi_SOURCE_DIR}/src/qml;${Akonadi_BINARY_DIR}/src/qml>")
target_include_directories(KPim${KF_MAJOR_VERSION}AkonadiQml PUBLIC "$<BUILD_INTERFACE:${Akonadi_BINARY_DIR};${Akonadi_SOURCE_DIR}/src/qml>")

target_link_libraries(KPim${KF_MAJOR_VERSION}AkonadiQml
PUBLIC
    Qt::Qml
    Qt::Quick
    KPim${KF_MAJOR_VERSION}::AkonadiCore
PRIVATE
    KF${KF_MAJOR_VERSION}::ConfigGui
)

set_target_properties(KPim${KF_MAJOR_VERSION}AkonadiQml PROPERTIES
    VERSION ${AKONADI_VERSION}
    SOVERSION ${AKONADI_SOVERSION}
    EXPORT_NAME AkonadiQml
)

ecm_generate_pri_file(BASE_NAME AkonadiQml
    LIB_NAME KPim${KF_MAJOR_VERSION}AkonadiQml
    DEPS "KDeclarative" FILENAME_VAR PRI_FILENAME
)

install(TARGETS
    KPim${KF_MAJOR_VERSION}AkonadiQml
    EXPORT KPim${KF_MAJOR_VERSION}AkonadiTargets ${KDE_INSTALL_TARGETS_DEFAULT_ARGS}
)

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/akonadiqml_export.h
    ${AkonadiCore_HEADERS}
    DESTINATION ${AkonadiQml_INSTALL_INCLUDEDIR}/akonadiqml
    COMPONENT Devel
)

install(FILES
    ${AkonadiCore_CC_HEADERS}
    DESTINATION ${AkonadiQml_INSTALL_INCLUDEDIR}/AkonadiQml
    COMPONENT Devel
)

install(FILES
    ${PRI_FILENAME}
    DESTINATION ${ECM_MKSPECS_INSTALL_DIR}
)
