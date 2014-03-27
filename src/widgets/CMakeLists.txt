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

qt5_wrap_ui(akonadiwidgets_SRCS ${akonadiwidgets_UI})

add_library(KF5AkonadiWidgets ${akonadiwidgets_SRCS})

generate_export_header(KF5AkonadiWidgets BASE_NAME akonadiagentbase)

add_library(KF5::AkonadiWidgets ALIAS KF5AkonadiWidgets)

target_include_directories(KF5AkonadiWidgets INTERFACE "$<INSTALL_INTERFACE:${INCLUDE_INSTALL_DIR}/Akonadi;${Boost_INCLUDE_DIR}>")

target_link_libraries(KF5AkonadiWidgets
    KF5::AkonadiCore
    KF5::ItemModels
    KF5::ItemViews ## This is only for KFilterProxySearchLine
    KF5::KDE4Support
)

set_target_properties(KF5AkonadiWidgets PROPERTIES
    VERSION ${AKONADI_VERSION_STRING}
    SOVERSION ${AKONADI_SOVERSION}
    EXPORT_NAME AkonadiWidgets
)

ecm_generate_pri_file(BASE_NAME AkonadiWidgets
    LIB_NAME KF5AkonadiWidgets
    DEPS "AkonadiCore KDE4Support" FILENAME_VAR PRI_FILENAME
)

install(TARGETS
    KF5AkonadiWidgets
    EXPORT KF5AkonadiTargets ${INSTALL_TARGETS_DEFAULT_ARGS}
)

install(FILES
    ${AkonadiWidgets_HEADERS}
    DESTINATION ${INCLUDE_INSTALL_DIR}/AkonadiWidgets
    COMPONENT Devel
)

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/akonadiwidgets_export.h
    ${AkonadiWidgets_HEADERS}
    DESTINATION ${INCLUDE_INSTALL_DIR}/AkonadiWidgets COMPONENT Devel
)

if (KDEPIM_MOBILE_UI)
    install(FILES
        CollectionDialogMobile.qml
        DESTINATION ${DATA_INSTALL_DIR}/akonadi/qml
    )
endif()

install(FILES
    ${PRI_FILENAME}
    DESTINATION ${ECM_MKSPECS_INSTALL_DIR}
)
