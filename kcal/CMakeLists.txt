include_directories(
  ${CMAKE_SOURCE_DIR}/
  ${QT_QTDBUS_INCLUDE_DIR}
)

set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DQT_NO_CAST_FROM_ASCII -DQT_NO_CAST_TO_ASCII ${KDE4_ENABLE_EXCEPTIONS}" )

########### next target ###############

if (NOT KDEPIM_NO_KCAL)
set( kcalakonadi_LIB_SRC
  freebusyproviderbase.cpp
  incidencemimetypevisitor.cpp
)

add_definitions(
  -DKCAL_DEPRECATED=
)
# This one won't be needed when CMake 2.8.13 is depended on.
add_definitions(
  -DKCAL_DEPRECATED_EXPORT=KCAL_EXPORT
)

set(akonadi_xml ${AKONADI_DBUS_INTERFACES_DIR}/org.freedesktop.Akonadi.NotificationSource.xml)
set_source_files_properties(${akonadi_xml} PROPERTIES INCLUDE "notificationmessage_p.h")
qt4_add_dbus_adaptor( kcalakonadi_LIB_SRC ../interfaces/org.freedesktop.Akonadi.Resource.FreeBusyProvider.xml freebusyproviderbase_p.h Akonadi::FreeBusyProviderBasePrivate freebusyprovideradaptor Akonadi__FreeBusyProviderAdaptor )

include_directories(
  ${CMAKE_BINARY_DIR}/kcal
)

add_library( akonadi-kcal ${LIBRARY_TYPE} ${kcalakonadi_LIB_SRC} )
if(CMAKE_MINIMUM_REQUIRED_VERSION VERSION_LESS 2.8.12)
  set_property(TARGET akonadi-kcal PROPERTY DEFINE_SYMBOL akonadi_kcal)
endif()
generate_export_header(akonadi-kcal
  BASE_NAME akonadi_kcal
  EXPORT_FILE_NAME akonadi-kcal_export.h
)

target_link_libraries( akonadi-kcal kcal akonadi-kde ${QT_QTGUI_LIBRARY} ${KDE4_KDECORE_LIBS} )
set_target_properties( akonadi-kcal PROPERTIES VERSION ${GENERIC_LIB_VERSION} SOVERSION ${GENERIC_LIB_SOVERSION} )
install(TARGETS akonadi-kcal EXPORT kdepimlibsLibraryTargets ${INSTALL_TARGETS_DEFAULT_ARGS})

########### install files ###############

install( FILES
  ${CMAKE_CURRENT_BINARY_DIR}/akonadi-kcal_export.h
  freebusyproviderbase.h
  incidencemimetypevisitor.h
  DESTINATION ${INCLUDE_INSTALL_DIR}/akonadi/kcal COMPONENT Devel
)
endif()
