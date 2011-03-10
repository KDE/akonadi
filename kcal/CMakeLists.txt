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

set(akonadi_xml ${AKONADI_DBUS_INTERFACES_DIR}/org.freedesktop.Akonadi.NotificationSource.xml)
set_source_files_properties(${akonadi_xml} PROPERTIES INCLUDE "notificationmessage_p.h")
qt4_add_dbus_adaptor( kcalakonadi_LIB_SRC ../interfaces/org.freedesktop.Akonadi.Resource.FreeBusyProvider.xml freebusyproviderbase_p.h Akonadi::FreeBusyProviderBasePrivate freebusyprovideradaptor Akonadi__FreeBusyProviderAdaptor )

kde4_add_library( akonadi-kcal ${LIBRARY_TYPE} ${kcalakonadi_LIB_SRC} )

target_link_libraries( akonadi-kcal kcal akonadi-kde ${QT_QTGUI_LIBRARY} ${KDE4_KDECORE_LIBS} )
set_target_properties( akonadi-kcal PROPERTIES VERSION ${GENERIC_LIB_VERSION} SOVERSION ${GENERIC_LIB_SOVERSION} )
install(TARGETS akonadi-kcal EXPORT kdepimlibsLibraryTargets ${INSTALL_TARGETS_DEFAULT_ARGS})

########### install files ###############

install( FILES
  akonadi-kcal_export.h
  freebusyproviderbase.h
  incidencemimetypevisitor.h
  DESTINATION ${INCLUDE_INSTALL_DIR}/akonadi/kcal COMPONENT Devel
)
endif (NOT KDEPIM_NO_KCAL)
