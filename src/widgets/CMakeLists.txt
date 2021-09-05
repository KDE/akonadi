if (BUILD_QCH)
    ecm_install_qch_export(
        TARGETS KF5AkonadiWidgets_QCH
        FILE KF5AkonadiWidgetsQchTargets.cmake
        DESTINATION "${CMAKECONFIG_INSTALL_DIR}"
        COMPONENT Devel
    )
    set(PACKAGE_INCLUDE_QCHTARGETS "include(\"\${CMAKE_CURRENT_LIST_DIR}/KF5AkonadiWidgetsQchTargets.cmake\")")
endif()

configure_file(akonadiwidgetstests_export.h.in "${CMAKE_CURRENT_BINARY_DIR}/akonadiwidgetstests_export.h")

# base install include dir
set(AkonadiWidgets_INSTALL_INCLUDEDIR "${KDE_INSTALL_INCLUDEDIR_KF5}/AkonadiWidgets")

add_library(KF5AkonadiWidgets)
add_library(KF5::AkonadiWidgets ALIAS KF5AkonadiWidgets)


target_sources(KF5AkonadiWidgets PRIVATE
    actionstatemanager.cpp
    agentactionmanager.cpp
    agentconfigurationdialog.cpp
    agentconfigurationwidget.cpp
    agentinstancewidget.cpp
    agenttypedialog.cpp
    agenttypewidget.cpp
    cachepolicypage.cpp
    collectioncombobox.cpp
    collectiondialog.cpp
    collectiongeneralpropertiespage.cpp
    collectionmaintenancepage.cpp
    collectionpropertiesdialog.cpp
    collectionpropertiespage.cpp
    collectionrequester.cpp
    collectionstatisticsdelegate.cpp
    collectionview.cpp
    conflictresolvedialog.cpp
    controlgui.cpp
    dragdropmanager.cpp
    entitylistview.cpp
    entitytreeview.cpp
    erroroverlay.cpp
    etmviewstatesaver.cpp
    itemview.cpp
    manageaccountwidget.cpp
    progressspinnerdelegate.cpp
    recentcollectionaction.cpp
    renamefavoritedialog.cpp
    standardactionmanager.cpp
    selftestdialog.cpp
    subscriptiondialog.cpp
    tageditwidget.cpp
    tagmanagementdialog.cpp
    tagselectioncombobox.cpp
    tagselectiondialog.cpp
    tagwidget.cpp
    tagselectwidget.cpp
)

ecm_qt_declare_logging_category(KF5AkonadiWidgets HEADER akonadiwidgets_debug.h IDENTIFIER AKONADIWIDGETS_LOG CATEGORY_NAME org.kde.pim.akonadiwidgets
        DESCRIPTION "akonadi (Akonadi Widget Library)"
        OLD_CATEGORY_NAMES akonadiwidgets_log
        EXPORT AKONADI
    )


set(akonadiwidgets_UI
    cachepolicypage.ui
    collectiongeneralpropertiespage.ui
    collectionmaintenancepage.ui
    controlprogressindicator.ui
    erroroverlay.ui
    manageaccountwidget.ui
    renamefavoritedialog.ui
    selftestdialog.ui
    subscriptiondialog.ui
    tageditwidget.ui
    tagmanagementdialog.ui
    tagselectiondialog.ui
    tagwidget.ui
)

akonadi_generate_headers_with_deprecated(AkonadiWidgets_CC_HEADERS
    HEADER_NAMES
    AgentActionManager
    AgentConfigurationDialog
    AgentConfigurationWidget
    AgentInstanceWidget
    AgentTypeDialog
    AgentTypeWidget
    CollectionComboBox
    CollectionDialog
    CollectionPropertiesDialog
    CollectionPropertiesPage
    CollectionMaintenancePage
    CollectionRequester
    CollectionStatisticsDelegate
    CollectionView
    ControlGui
    EntityListView
    EntityTreeView
    ETMViewStateSaver
    ItemView
    ManageAccountWidget
    StandardActionManager
    SubscriptionDialog
    TagManagementDialog
    TagSelectionComboBox
    TagSelectionDialog
    TagEditWidget
    TagWidget
    TagSelectWidget
    REQUIRED_HEADERS AkonadiWidgets_HEADERS
    DEPRECATED_DESTINATION ${AkonadiWidgets_INSTALL_INCLUDEDIR}
)

ki18n_wrap_ui(KF5AkonadiWidgets ${akonadiwidgets_UI})
if (COMPILE_WITH_UNITY_CMAKE_SUPPORT)
    set_target_properties(KF5AkonadiWidgets PROPERTIES UNITY_BUILD ON)
endif()

generate_export_header(KF5AkonadiWidgets BASE_NAME akonadiwidgets)


target_include_directories(KF5AkonadiWidgets INTERFACE "$<INSTALL_INTERFACE:${AkonadiWidgets_INSTALL_INCLUDEDIR}>")

target_link_libraries(KF5AkonadiWidgets
PUBLIC
    KF5::AkonadiCore
    KF5::ItemModels
    Qt::Widgets
    KF5::ConfigWidgets
PRIVATE
    KF5::I18n
    KF5::IconThemes
    KF5::XmlGui
    KF5::ItemViews
    Qt::Sql
    KF5::AkonadiPrivate
    KF5::WindowSystem
)

set_target_properties(KF5AkonadiWidgets PROPERTIES
    VERSION ${AKONADI_VERSION}
    SOVERSION ${AKONADI_SOVERSION}
    EXPORT_NAME AkonadiWidgets
)

ecm_generate_pri_file(BASE_NAME AkonadiWidgets
    LIB_NAME KF5AkonadiWidgets
    DEPS "AkonadiCore KItemModels" FILENAME_VAR PRI_FILENAME
)

install(TARGETS
    KF5AkonadiWidgets
    EXPORT KF5AkonadiTargets ${KF5_INSTALL_TARGETS_DEFAULT_ARGS}
)

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/akonadiwidgets_export.h
    ${AkonadiWidgets_HEADERS}
    DESTINATION ${AkonadiWidgets_INSTALL_INCLUDEDIR}/akonadi
    COMPONENT Devel
)
install(FILES
    ${AkonadiWidgets_CC_HEADERS}
    DESTINATION ${AkonadiWidgets_INSTALL_INCLUDEDIR}/Akonadi
    COMPONENT Devel
)

install(FILES
    ${PRI_FILENAME}
    DESTINATION ${ECM_MKSPECS_INSTALL_DIR}
)


######### Build and install QtDesigner plugin #############

if(BUILD_DESIGNERPLUGIN)
    include(ECMAddQtDesignerPlugin)
    ecm_qtdesignerplugin_widget(Akonadi::AgentInstanceWidget
        INCLUDE_FILE "agentinstancewidget.h"
        TOOLTIP "Akonadi Agent Instance Settings (Akonadi)"
        WHATSTHIS "A widget to configure akonadi instance."
        GROUP "Input (KDE-PIM)"
    )
    ecm_qtdesignerplugin_widget(Akonadi::EntityTreeView
        INCLUDE_FILE "entitytreeview.h"
        TOOLTIP "A view to show an item/collection tree provided by an EntityTreeModel (Akonadi)"
        WHATSTHIS "A view to show an item/collection tree provided by an EntityTreeModel."
        GROUP "Input (KDE-PIM)"
    )
    ecm_qtdesignerplugin_widget(Akonadi::TagSelectWidget
        INCLUDE_FILE "tagselectwidget.h"
        TOOLTIP "A Widget to select tag (Akonadi)"
        WHATSTHIS "A Widget to select tag."
        GROUP "Input (KDE-PIM)"
    )
    ecm_qtdesignerplugin_widget(Akonadi::ManageAccountWidget
        INCLUDE_FILE "manageaccountwidget.h"
        TOOLTIP "A Widget to Manage Akonadi Account (KDE-PIM)"
        WHATSTHIS "A Widget to Manage Akonadi Account."
        GROUP "Input (KDE-PIM)"
    )

    ecm_add_qtdesignerplugin(akonadiwidgets
        NAME AkonadiWidgets
        OUTPUT_NAME akonadiwidgets
        WIDGETS
        Akonadi::AgentInstanceWidget
        Akonadi::EntityTreeView
        Akonadi::TagSelectWidget
        Akonadi::ManageAccountWidget
        LINK_LIBRARIES
        KF5::AkonadiCore
        KF5::AkonadiWidgets
        INSTALL_DESTINATION "${KDE_INSTALL_QTPLUGINDIR}/designer"
        COMPONENT Devel
    )
endif()

if (BUILD_QCH)
    ecm_add_qch(
        KF5AkonadiWidgets_QCH
        NAME KF5AkonadiWidgets
        BASE_NAME KF5AkonadiWidgets
        VERSION ${PIM_VERSION}
        ORG_DOMAIN org.kde
        SOURCES # using only public headers, to cover only public API
            ${AkonadiWidgets_HEADERS}
        #MD_MAINPAGE "${CMAKE_SOURCE_DIR}/README.md"
        #IMAGE_DIRS "${CMAKE_SOURCE_DIR}/docs/pics"
        LINK_QCHS
            Qt5Core_QCH
            Qt5Gui_QCH
            Qt5Widgets_QCH
        INCLUDE_DIRS
            ${CMAKE_CURRENT_BINARY_DIR}
        BLANK_MACROS
            AKONADIWIDGETS_EXPORT
        TAGFILE_INSTALL_DESTINATION ${KDE_INSTALL_QTQCHDIR}
        QCH_INSTALL_DESTINATION ${KDE_INSTALL_QTQCHDIR}
        COMPONENT Devel
    )
endif()
