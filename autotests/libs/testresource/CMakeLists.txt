include(${CMAKE_SOURCE_DIR}/KPimAkonadiMacros.cmake)

kde_enable_exceptions()

remove_definitions(-DTRANSLATION_DOMAIN=\"libakonadi6\")
add_definitions(-DTRANSLATION_DOMAIN=\"akonadi_knut_resource\")

# Disabled for now, resourcetester remained in kdepim-runtime
#add_subdirectory(tests)

################################# Common  ####################################

kconfig_add_kcfg_files(knut_common_SRCS settings.kcfgc)
kcfg_generate_dbus_interface(${CMAKE_CURRENT_SOURCE_DIR}/knutresource.kcfg org.kde.Akonadi.Knut.Settings)

qt_add_dbus_adaptor(knut_common_SRCS
  ${CMAKE_CURRENT_BINARY_DIR}/org.kde.Akonadi.Knut.Settings.xml settings.h KnutSettings
)

ecm_qt_declare_logging_category(knut_common_SRCS
    HEADER knutresource_debug.h
    IDENTIFIER KNUTRESOURCE_LOG
    CATEGORY_NAME org.kde.pim.knut
)

################################# Resource ####################################

add_executable(akonadi_knut_resource
    knutresource.cpp
    knutresource.h
    ${knut_common_SRCS}
)
set_target_properties(akonadi_knut_resource PROPERTIES MACOSX_BUNDLE FALSE)

target_link_libraries(akonadi_knut_resource PUBLIC
  KPim6::AkonadiXml
  KPim6::AkonadiCore
  KPim6::AkonadiAgentBase
  Qt::Xml
  KF6::I18n
)

install(TARGETS akonadi_knut_resource ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})
install(FILES knutresource.desktop DESTINATION "${KDE_INSTALL_DATAROOTDIR}/akonadi/agents")
install(FILES knut-template.xml DESTINATION ${KDE_INSTALL_DATADIR_KF}/akonadi_knut_resource/)

################################## Config plugin ##############################

kcoreaddons_add_plugin(knutconfig
    INSTALL_NAMESPACE "pim6/akonadi/config"
)

target_sources(knutconfig PRIVATE
    knutconfig.cpp
    accountwidget.cpp
    accountwidget.h
    ${knut_common_SRCS}
)

target_link_libraries(knutconfig
    Qt::Widgets
    KF6::I18n
    KPim6::AkonadiCore
    KPim6::AkonadiAgentBase
)
