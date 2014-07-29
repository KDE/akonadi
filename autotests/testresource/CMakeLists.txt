include_directories(
  ${CMAKE_CURRENT_BINARY_DIR}
  ${AKONADI_INCLUDE_DIR}
  ${AKONADI_INCLUDE_DIR}/akonadi/private
  ${Boost_INCLUDE_DIR}
  ${CMAKE_SOURCE_DIR}/akonadi
  ${CMAKE_BINARY_DIR}/akonadi
  ${CMAKE_BINARY_DIR}/akonadi/kmime
  ${CMAKE_BINARY_DIR}/kmime
)

kde_enable_exceptions()

find_package(LibXslt)
set_package_properties(LibXslt PROPERTIES DESCRIPTION "xsltproc" URL "http://xmlsoft.org/XSLT/" TYPE REQUIRED PURPOSE "Needed to generate D-Bus interface specifications")

find_program(XSLTPROC_EXECUTABLE xsltproc DOC "Path to the xsltproc executable")

# generates a D-Bus interface description from a KConfigXT file
macro(kcfg_generate_dbus_interface _kcfg _name)
  add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${_name}.xml
    COMMAND ${XSLTPROC_EXECUTABLE} --stringparam interfaceName ${_name}
    ${Akonadi_SOURCE_DIR}/src/core/kcfg2dbus.xsl
    ${_kcfg}
    > ${CMAKE_CURRENT_BINARY_DIR}/${_name}.xml
    DEPENDS ${Akonadi_SOURCE_DIR}/src/core/kcfg2dbus.xsl
    ${_kcfg}
    )
endmacro()

# Disabled for now, resourcetester remained in kdepim-runtime
#add_subdirectory( tests )

set( knutresource_SRCS knutresource.cpp knutresourcefactory.cpp)

kconfig_add_kcfg_files(knutresource_SRCS settings.kcfgc)

kcfg_generate_dbus_interface(${CMAKE_CURRENT_SOURCE_DIR}/knutresource.kcfg org.kde.Akonadi.Knut.Settings)

qt5_add_dbus_adaptor(knutresource_SRCS
  ${CMAKE_CURRENT_BINARY_DIR}/org.kde.Akonadi.Knut.Settings.xml settings.h KnutSettings
)

add_library(akonadi_knut_resource MODULE ${knutresource_SRCS})

if (Q_WS_MAC)
  set_target_properties(akonadi_knut_resource PROPERTIES MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/../Info.plist.template)
  set_target_properties(akonadi_knut_resource PROPERTIES MACOSX_BUNDLE_GUI_IDENTIFIER "org.kde.Akonadi.Knut")
  set_target_properties(akonadi_knut_resource PROPERTIES MACOSX_BUNDLE_BUNDLE_NAME "KDE Akonadi Knut Resource")
endif ()

target_link_libraries(akonadi_knut_resource
  KF5::AkonadiXml
  KF5::AkonadiCore  
  KF5::KIOCore
  KF5::AkonadiAgentBase
  Qt5::Xml
)

install( TARGETS akonadi_knut_resource DESTINATION ${PLUGIN_INSTALL_DIR}/ )
install( FILES knutresource.desktop DESTINATION "${CMAKE_INSTALL_PREFIX}/share/akonadi/agents" )
install( FILES knut-template.xml DESTINATION ${KF5_DATA_INSTALL_DIR}/akonadi_knut_resource/ )

