find_package(LibXml2)

if ( LIBXML2_FOUND )
  add_definitions( -DHAVE_LIBXML2 )
endif ( LIBXML2_FOUND )

include_directories(
    ${Boost_INCLUDE_DIR}
    ${LIBXML2_INCLUDE_DIR}
)

set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${KDE4_ENABLE_EXCEPTIONS}" )

set( knutresource_SRCS
  knutresource.cpp
)

kde4_add_kcfg_files(knutresource_SRCS settings.kcfgc)
kcfg_generate_dbus_interface(${CMAKE_CURRENT_SOURCE_DIR}/knutresource.kcfg org.kde.Akonadi.Knut.Settings)
qt4_add_dbus_adaptor(knutresource_SRCS
  ${CMAKE_CURRENT_BINARY_DIR}/org.kde.Akonadi.Knut.Settings.xml settings.h Settings
)
kde4_add_executable(akonadi_knut_resource RUN_UNINSTALLED ${knutresource_SRCS})

target_link_libraries(akonadi_knut_resource
  ${KDE4_AKONADI_LIBS}
  ${QT_QTXML_LIBRARY}
  ${LIBXML2_LIBRARIES}
)

install( TARGETS akonadi_knut_resource ${INSTALL_TARGETS_DEFAULT_ARGS} )
install( FILES knutresource.desktop DESTINATION "${CMAKE_INSTALL_PREFIX}/share/akonadi/agents" )
install( FILES knut.xsd DESTINATION ${DATA_INSTALL_DIR}/akonadi_knut_resource/ )
