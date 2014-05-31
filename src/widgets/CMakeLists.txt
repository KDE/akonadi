# Turn exceptions on
kde_enable_exceptions()

set(akonadiwidgets_SRCS
    actionstatemanager.cpp
    agentactionmanager.cpp
    agentinstancewidget.cpp
    agenttypedialog.cpp
    agenttypewidget.cpp
    cachepolicypage.cpp
    collectioncombobox.cpp
    collectiongeneralpropertiespage.cpp
    collectionpropertiesdialog.cpp
    collectionpropertiespage.cpp
    collectionrequester.cpp
    collectionstatisticsdelegate.cpp
    collectionview.cpp
    conflictresolvedialog.cpp
    dragdropmanager.cpp
    entitylistview.cpp
    entitytreeview.cpp
    entitytreeviewstatesaver.cpp
    erroroverlay.cpp
    etmviewstatesaver.cpp
    itemview.cpp
    progressspinnerdelegate.cpp
    recentcollectionaction.cpp
    renamefavoritedialog.cpp
    standardactionmanager.cpp
    subscriptiondialog.cpp
    tageditwidget.cpp
    tagmanagementdialog.cpp
    tagselectiondialog.cpp
    tagwidget.cpp
)

set(akonadiwidgets_UI
    cachepolicypage.ui
    controlprogressindicator.ui
    erroroverlay.ui
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
    CollectionRequester
    CollectionStatisticsDelegate
    CollectionView
    EntityListView
    EntityTreeView
    EntityTreeViewStateSaver
    ETMViewStateSaver
    ItemView
    RenameFavoriteDialog
    StandardActionManager
    SubscriptionDialog
    TagManagementDialog
    TagSelectionDialog
    TagWidget
    REQUIRED_HEADERS AkonadiWidgets_HEADERS
)

if (KDEPIM_MOBILE_UI)
    set(akonadiwidgets_SRCS
        ${akonadiwidgets_SRCS}
        collectiondialog_mobile.cpp
    )
    set(akonadiwidgets_UI
        ${akonadiwidgets_UI}
        collectiongeneralpropertiespage_mobile.ui
    )
else()
    set(akonadiwidgets_SRCS
        ${akonadiwidgets_SRCS}
        collectiondialog_desktop.cpp
    )
    set(akonadiwidgets_UI
        ${akonadiwidgets_UI}
        collectiongeneralpropertiespage.ui
    )
endif()

ki18n_wrap_ui(akonadiwidgets_SRCS ${akonadiwidgets_UI})

add_library(KF5AkonadiWidgets ${akonadiwidgets_SRCS})

generate_export_header(KF5AkonadiWidgets BASE_NAME akonadiwidgets)

add_library(KF5::AkonadiWidgets ALIAS KF5AkonadiWidgets)

target_include_directories(KF5AkonadiWidgets INTERFACE "$<INSTALL_INTERFACE:${KF5_INCLUDE_INSTALL_DIR}/AkonadiWidgets;${Boost_INCLUDE_DIR}>")

target_link_libraries(KF5AkonadiWidgets
    KF5::AkonadiCore
    KF5::ItemModels
    KF5::ItemViews ## This is only for KFilterProxySearchLine
    KF5::KDELibs4Support
)

set_target_properties(KF5AkonadiWidgets PROPERTIES
    VERSION ${AKONADILIBRARIES_VERSION_STRING}
    SOVERSION ${AKONADILIBRARIES_SOVERSION}
    EXPORT_NAME AkonadiWidgets
)

ecm_generate_pri_file(BASE_NAME AkonadiWidgets
    LIB_NAME KF5AkonadiWidgets
    DEPS "AkonadiCore KDELibs4Support" FILENAME_VAR PRI_FILENAME
)

install(TARGETS
    KF5AkonadiWidgets
    EXPORT KF5AkonadiTargets ${KF5_INSTALL_TARGETS_DEFAULT_ARGS}
)

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/akonadiwidgets_export.h
    ${AkonadiWidgets_HEADERS}
    DESTINATION ${KF5_INCLUDE_INSTALL_DIR}/AkonadiWidgets COMPONENT Devel
)

if (KDEPIM_MOBILE_UI)
    install(FILES
        CollectionDialogMobile.qml
        DESTINATION ${KF5_DATA_INSTALL_DIR}/akonadi/qml
    )
endif()

install(FILES
    ${PRI_FILENAME}
    DESTINATION ${ECM_MKSPECS_INSTALL_DIR}
)

######### Build and install QtDesigner plugin #############

if (Qt5Designer_FOUND)
    include_directories(${Qt5Designer_INCLUDE_DIRS})

    add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/akonadiwidgets.cpp
        COMMAND kgendesignerplugin -o ${CMAKE_CURRENT_BINARY_DIR}/akonadiwidgets.cpp ${CMAKE_CURRENT_SOURCE_DIR}/akonadi.widgets
        MAIN_DEPENDENCY akonadi.widgets)

    set(akonadiwidgets_SRCS
        ${CMAKE_CURRENT_BINARY_DIR}/akonadiwidgets.cpp
        ${CMAKE_CURRENT_BINARY_DIR}/akonadiwidgets.moc
    )

    qt5_generate_moc(${CMAKE_CURRENT_BINARY_DIR}/akonadiwidgets.cpp
        ${CMAKE_CURRENT_BINARY_DIR}/akonadiwidgets.moc
    )

    add_library(akonadiwidgets MODULE ${akonadiwidgets_SRCS})
    target_link_libraries(akonadiwidgets
        KF5::AkonadiCore
        KF5::AkonadiWidgets
    )

    if(NOT WIN32)
        set_target_properties(akonadiwidgets PROPERTIES
            INSTALL_RPATH_USE_LINK_PATH TRUE
            SKIP_BUILD_RPATH TRUE
            BUILD_WITH_INSTALL_RPATH TRUE
            INSTALL_RPATH ${LIB_INSTALL_DIR}
        )
    endif()

    install(TARGETS
        akonadiwidgets
        DESTINATION ${QT_PLUGIN_INSTALL_DIR}/designer
    )
endif()
