set(akonadiwidgets_SRCS
    actionstatemanager.cpp
    agentactionmanager.cpp
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

ecm_qt_declare_logging_category(akonadiwidgets_SRCS HEADER akonadiwidgets_debug.h IDENTIFIER AKONADIWIDGETS_LOG CATEGORY_NAME org.kde.pim.akonadiwidgets)

set(akonadiwidgets_UI
    cachepolicypage.ui
    collectiongeneralpropertiespage.ui
    collectionmaintenancepage.ui
    controlprogressindicator.ui
    erroroverlay.ui
    manageaccountwidget.ui
    selftestdialog.ui
)

ecm_generate_headers(AkonadiWidgets_HEADERS
    HEADER_NAMES
    AgentActionManager
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
    RenameFavoriteDialog
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
    KF5::DBusAddons
    Qt5::Sql
    KF5::AkonadiPrivate
    KF5::WindowSystem
    KF5::Completion
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

if (KF5DesignerPlugin_FOUND)
    kf5designerplugin_add_plugin(akonadiwidgets akonadi.widgets)
    target_link_libraries(akonadiwidgets
        KF5::AkonadiCore
        KF5::AkonadiWidgets
    )
    set_target_properties(akonadiwidgets PROPERTIES
        OUTPUT_NAME akonadi5widgets
    )

    if(NOT WIN32)
        # Since there are no libraries provided by this module,
        # there is no point including the build tree in RPath,
        # and then having to edit it at install time.
        set_target_properties(akonadiwidgets PROPERTIES
            SKIP_BUILD_RPATH TRUE
            BUILD_WITH_INSTALL_RPATH TRUE
        )
    endif()

    install(TARGETS
        akonadiwidgets
        DESTINATION ${KDE_INSTALL_QTPLUGINDIR}/designer
    )
endif()
