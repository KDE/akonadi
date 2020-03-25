set(akonadiwidgets_SRCS
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
    tagselectiondialog.cpp
    tagwidget.cpp
    tagselectwidget.cpp
)

ecm_qt_declare_logging_category(akonadiwidgets_SRCS HEADER akonadiwidgets_debug.h IDENTIFIER AKONADIWIDGETS_LOG CATEGORY_NAME org.kde.pim.akonadiwidgets
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
    selftestdialog.ui
    subscriptiondialog.ui
    tageditwidget.ui
    tagmanagementdialog.ui
    tagselectiondialog.ui
    tagwidget.ui
)

ecm_generate_headers(AkonadiWidgets_HEADERS
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
    TagSelectionDialog
    TagEditWidget
    TagWidget
    TagSelectWidget
    REQUIRED_HEADERS AkonadiWidgets_HEADERS
)

ki18n_wrap_ui(akonadiwidgets_SRCS ${akonadiwidgets_UI})

add_library(KF5AkonadiWidgets ${akonadiwidgets_SRCS})

generate_export_header(KF5AkonadiWidgets BASE_NAME akonadiwidgets)

add_library(KF5::AkonadiWidgets ALIAS KF5AkonadiWidgets)

target_include_directories(KF5AkonadiWidgets INTERFACE "$<INSTALL_INTERFACE:${KDE_INSTALL_INCLUDEDIR_KF5}/AkonadiWidgets;${Boost_INCLUDE_DIR}>")

target_link_libraries(KF5AkonadiWidgets
PUBLIC
    KF5::AkonadiCore
    KF5::ItemModels
    Qt5::Widgets
    KF5::ConfigWidgets
PRIVATE
    KF5::I18n
    KF5::IconThemes
    KF5::XmlGui
    KF5::ItemViews
    Qt5::Sql
    KF5::AkonadiPrivate
    KF5::WindowSystem
)

set_target_properties(KF5AkonadiWidgets PROPERTIES
    VERSION ${AKONADI_VERSION_STRING}
    SOVERSION ${AKONADI_SOVERSION}
    EXPORT_NAME AkonadiWidgets
)

ecm_generate_pri_file(BASE_NAME AkonadiWidgets
    LIB_NAME KF5AkonadiWidgets
    DEPS "AkonadiCore ItemModels" FILENAME_VAR PRI_FILENAME
)

install(TARGETS
    KF5AkonadiWidgets
    EXPORT KF5AkonadiTargets ${KF5_INSTALL_TARGETS_DEFAULT_ARGS}
)

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/akonadiwidgets_export.h
    ${AkonadiWidgets_HEADERS}
    DESTINATION ${KDE_INSTALL_INCLUDEDIR_KF5}/AkonadiWidgets COMPONENT Devel
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
