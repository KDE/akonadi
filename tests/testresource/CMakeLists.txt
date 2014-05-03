set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${KDE4_ENABLE_EXCEPTIONS}" )

# Disabled for now, resourcetester remained in kdepim-runtime
#add_subdirectory( tests )

set( knutresource_SRCS knutresource.cpp )

kde4_add_kcfg_files(knutresource_SRCS settings.kcfgc)

kcfg_generate_dbus_interface(${CMAKE_CURRENT_SOURCE_DIR}/knutresource.kcfg org.kde.Akonadi.Knut.Settings)

qt4_add_dbus_adaptor(knutresource_SRCS
  ${CMAKE_CURRENT_BINARY_DIR}/org.kde.Akonadi.Knut.Settings.xml settings.h KnutSettings
)

kde4_add_plugin(akonadi_knut_resource ${knutresource_SRCS})

if (Q_WS_MAC)
  set_target_properties(akonadi_knut_resource PROPERTIES MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/../Info.plist.template)
  set_target_properties(akonadi_knut_resource PROPERTIES MACOSX_BUNDLE_GUI_IDENTIFIER "org.kde.Akonadi.Knut")
  set_target_properties(akonadi_knut_resource PROPERTIES MACOSX_BUNDLE_BUNDLE_NAME "KDE Akonadi Knut Resource")
endif ()

target_link_libraries(akonadi_knut_resource
  akonadi-xml
  akonadi-kde
  ${KDE4_KIO_LIBS}
  ${QT_QTXML_LIBRARY}
)

install( TARGETS akonadi_knut_resource DESTINATION ${PLUGIN_INSTALL_DIR}/ )
install( FILES knutresource.desktop DESTINATION "${CMAKE_INSTALL_PREFIX}/share/akonadi/agents" )
install( FILES knut-template.xml DESTINATION ${KF5_DATA_INSTALL_DIR}/akonadi_knut_resource/ )

