include(${CMAKE_SOURCE_DIR}/KF5AkonadiMacros.cmake)

kde_enable_exceptions()

remove_definitions(-DTRANSLATION_DOMAIN=\"libakonadi5\")
add_definitions(-DTRANSLATION_DOMAIN=\"akonadi_knut_resource\")

# Disabled for now, resourcetester remained in kdepim-runtime
#add_subdirectory( tests )

set( knutresource_SRCS knutresource.cpp)

ecm_qt_declare_logging_category(knutresource_SRCS HEADER knutresource_debug.h IDENTIFIER KNUTRESOURCE_LOG CATEGORY_NAME org.kde.pim.knut)


kconfig_add_kcfg_files(knutresource_SRCS settings.kcfgc)

kcfg_generate_dbus_interface(${CMAKE_CURRENT_SOURCE_DIR}/knutresource.kcfg org.kde.Akonadi.Knut.Settings)

qt5_add_dbus_adaptor(knutresource_SRCS
  ${CMAKE_CURRENT_BINARY_DIR}/org.kde.Akonadi.Knut.Settings.xml settings.h KnutSettings
)

add_executable(akonadi_knut_resource ${knutresource_SRCS})
set_target_properties(akonadi_knut_resource PROPERTIES MACOSX_BUNDLE FALSE)

target_link_libraries(akonadi_knut_resource
  KF5::AkonadiXml
  KF5::AkonadiCore
  KF5::KIOCore
  KF5::AkonadiAgentBase
  Qt::Xml
  KF5::I18n
)

install( TARGETS akonadi_knut_resource ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})
install( FILES knutresource.desktop DESTINATION "${KDE_INSTALL_DATAROOTDIR}/akonadi/agents" )
install( FILES knut-template.xml DESTINATION ${KDE_INSTALL_DATADIR_KF5}/akonadi_knut_resource/ )
