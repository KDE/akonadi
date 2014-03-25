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

add_library(KF5AkonadiWidgets ${akonadiwidgets_SRCS})

generate_export_header(KF5AkonadiWidgets BASE_NAME akonadiwidgets)

add_library(KF5::AkonadiWidgets ALIAS KF5AkonadiWidgets)

target_include_directories(KF5AkonadiWidgets INTERFACE "$<INSTALL_INTERFACE:${INCLUDE_INSTALL_DIR}/akonadi;${Boost_INCLUDE_DIR}>")
target_include_directories(KF5AkonadiWidgets PUBLIC "$<BUILD_INTERFACE:${AkonadiWidgets_SOURCE_DIR}/src;${AkonadiWidgets_BINARY_DIR}/src>")
target_include_directories(KF5AkonadiWidgets PUBLIC "$<BUILD_INTERFACE:${AkonadiWidgets_SOURCE_DIR}/src/widgets;${AkonadiWidgets_BINARY_DIR}/src/widgets>")

target_link_libraries(KF5AkonadiWidgets
    KF5::ItemViews
    KF5::ItemModels
)

set_target_properties(KF5AkonadiWidgets PROPERTIES
    VERSION ${AKONADI_VERSION_STRING}
    SOVERSION ${AKONADI_SOVERSION}
    EXPORT_NAME AkonadiWidgets
)

# ecm_generate_headers(KSyndication_CamelCase_HEADERS
#   HEADER_NAMES
#   AbstractParser
#   REQUIRED_HEADERS KSyndication_HEADERS
#   PREFIX Syndication
# )


install(TARGETS KF5AkonadiWidgets EXPORT KF5AkonadiTargets ${INSTALL_TARGETS_DEFAULT_ARGS})

install(FILES
    #${KSyndication_CamelCase_HEADERS}
    DESTINATION ${INCLUDE_INSTALL_DIR}/Akonadi
    COMPONENT Devel
)

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/akonadiwidgets_export.h
    ${AkonadiWidgets_HEADERS}
    DESTINATION ${INCLUDE_INSTALL_DIR}/akonadi COMPONENT Devel)

ecm_generate_pri_file(BASE_NAME AkonadiWidgets LIB_NAME KF5AkonadiWidgets DEPS "ItemViews ItemModels" FILENAME_VAR PRI_FILENAME)
install(FILES ${PRI_FILENAME} DESTINATION ${ECM_MKSPECS_INSTALL_DIR})
