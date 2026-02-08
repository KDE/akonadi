configure_file(akonadiwidgetstests_export.h.in "${CMAKE_CURRENT_BINARY_DIR}/akonadiwidgetstests_export.h")

# base install include dir
set(AkonadiWidgets_INSTALL_INCLUDEDIR "${KDE_INSTALL_INCLUDEDIR}/KPim6/AkonadiWidgets")

add_library(KPim6AkonadiWidgets)
add_library(KPim6::AkonadiWidgets ALIAS KPim6AkonadiWidgets)

qt_add_dbus_interfaces(akonadiwidget_dbus_SRCS ${Akonadi_SOURCE_DIR}/src/interfaces/org.freedesktop.Akonadi.Agent.Control.xml)

target_sources(
    KPim6AkonadiWidgets
    PRIVATE
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
        conflictresolvedialog.cpp
        controlgui.cpp
        dragdropmanager.cpp
        entitylistview.cpp
        entitytreeview.cpp
        erroroverlay.cpp
        etmviewstatesaver.cpp
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
        actionstatemanager_p.h
        agentactionmanager.h
        agentconfigurationdialog.h
        agentconfigurationwidget.h
        agentinstancewidget.h
        agenttypedialog.h
        agenttypewidget.h
        cachepolicypage.h
        collectioncombobox.h
        collectiondialog.h
        collectiongeneralpropertiespage_p.h
        collectionmaintenancepage.h
        collectionpropertiesdialog.h
        collectionpropertiespage.h
        collectionrequester.h
        collectionstatisticsdelegate.h
        conflictresolvedialog_p.h
        controlgui.h
        dragdropmanager_p.h
        entitylistview.h
        entitytreeview.h
        erroroverlay_p.h
        etmviewstatesaver.h
        manageaccountwidget.h
        progressspinnerdelegate_p.h
        recentcollectionaction_p.h
        renamefavoritedialog_p.h
        standardactionmanager.h
        selftestdialog.h
        subscriptiondialog.h
        tageditwidget.h
        tagmanagementdialog.h
        tagselectioncombobox.h
        tagselectiondialog.h
        tagwidget.h
        tagselectwidget.h
        clearcachefoldersjob.cpp
        clearcachefoldersjob.h
        dbaccess.cpp
        dbaccess.h
        ${akonadiwidget_dbus_SRCS}
)

ecm_qt_declare_logging_category(KPim6AkonadiWidgets HEADER akonadiwidgets_debug.h IDENTIFIER AKONADIWIDGETS_LOG CATEGORY_NAME org.kde.pim.akonadiwidgets
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

ecm_generate_headers(AkonadiWidgets_CC_HEADERS
    HEADER_NAMES
    AgentActionManager
    AgentConfigurationDialog
    AgentConfigurationWidget
    AgentInstanceWidget
    AgentTypeDialog
    AgentTypeWidget
    ClearCacheFoldersJob
    CollectionComboBox
    CollectionDialog
    CollectionPropertiesDialog
    CollectionPropertiesPage
    CollectionMaintenancePage
    CollectionRequester
    CollectionStatisticsDelegate
    ControlGui
    DbAccess
    EntityListView
    EntityTreeView
    ETMViewStateSaver
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
    PREFIX Akonadi
)

ki18n_wrap_ui(KPim6AkonadiWidgets ${akonadiwidgets_UI})
if(COMPILE_WITH_UNITY_CMAKE_SUPPORT)
    set_target_properties(
        KPim6AkonadiWidgets
        PROPERTIES
            UNITY_BUILD
                ON
    )
endif()

generate_export_header(KPim6AkonadiWidgets BASE_NAME akonadiwidgets)

target_include_directories(KPim6AkonadiWidgets INTERFACE "$<INSTALL_INTERFACE:${AkonadiWidgets_INSTALL_INCLUDEDIR}>")

target_link_libraries(
    KPim6AkonadiWidgets
    PUBLIC
        KPim6::AkonadiCore
        KF6::ItemModels
        Qt::Widgets
        KF6::ConfigWidgets
    PRIVATE
        KF6::I18n
        KF6::IconThemes
        KF6::IconWidgets
        KF6::XmlGui
        KF6::ColorScheme
        Qt::Sql
        KPim6::AkonadiPrivate
)

set_target_properties(
    KPim6AkonadiWidgets
    PROPERTIES
        VERSION
            ${AKONADI_VERSION}
        SOVERSION
            ${AKONADI_SOVERSION}
        EXPORT_NAME
            AkonadiWidgets
)

install(TARGETS KPim6AkonadiWidgets EXPORT KPim6AkonadiTargets ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})

install(
    FILES
        ${CMAKE_CURRENT_BINARY_DIR}/akonadiwidgets_export.h
        ${AkonadiWidgets_HEADERS}
    DESTINATION ${AkonadiWidgets_INSTALL_INCLUDEDIR}/akonadi
    COMPONENT Devel
)
install(FILES ${AkonadiWidgets_CC_HEADERS} DESTINATION ${AkonadiWidgets_INSTALL_INCLUDEDIR}/Akonadi COMPONENT Devel)

######### Build and install QtDesigner plugin #############

if(BUILD_DESIGNERPLUGIN)
    include(ECMAddQtDesignerPlugin)
    ecm_qtdesignerplugin_widget(Akonadi::AgentInstanceWidget
	    INCLUDE_FILE "Akonadi/AgentInstanceWidget"
        TOOLTIP "Akonadi Agent Instance Settings (Akonadi)"
        WHATSTHIS "A widget to configure akonadi instance."
        GROUP "Input (KDE-PIM)"
    )
    ecm_qtdesignerplugin_widget(Akonadi::EntityTreeView
	    INCLUDE_FILE "Akonadi/EntityTreeView"
        TOOLTIP "A view to show an item/collection tree provided by an EntityTreeModel (Akonadi)"
        WHATSTHIS "A view to show an item/collection tree provided by an EntityTreeModel."
        GROUP "Input (KDE-PIM)"
    )
    ecm_qtdesignerplugin_widget(Akonadi::TagSelectWidget
	    INCLUDE_FILE "Akonadi/TagSelectWidget"
        TOOLTIP "A Widget to select tag (Akonadi)"
        WHATSTHIS "A Widget to select tag."
        GROUP "Input (KDE-PIM)"
    )
    ecm_qtdesignerplugin_widget(Akonadi::ManageAccountWidget
	    INCLUDE_FILE "Akonadi/ManageAccountWidget"
        TOOLTIP "A Widget to Manage Akonadi Account (KDE-PIM)"
        WHATSTHIS "A Widget to Manage Akonadi Account."
        GROUP "Input (KDE-PIM)"
    )

    ecm_add_qtdesignerplugin(akonadiwidgets
        NAME AkonadiWidgets
        OUTPUT_NAME akonadi6widgets
        WIDGETS
        Akonadi::AgentInstanceWidget
        Akonadi::EntityTreeView
        Akonadi::TagSelectWidget
        Akonadi::ManageAccountWidget
        LINK_LIBRARIES
        KPim6::AkonadiCore
        KPim6::AkonadiWidgets
        INSTALL_DESTINATION "${KDE_INSTALL_QTPLUGINDIR}/designer"
        COMPONENT Devel
    )
endif()
ecm_generate_qdoc(KPim6AkonadiWidgets akonadi-widgets.qdocconf)
