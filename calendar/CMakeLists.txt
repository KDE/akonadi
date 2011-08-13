
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -UQT_NO_CAST_FROM_ASCII -UQT_NO_CAST_TO_ASCII ${KDE4_ENABLE_EXCEPTIONS}")

set(akonadicalendar_LIB_SRC
  freebusyproviderbase.cpp
  standardcalendaractionmanager.cpp
)
qt4_add_dbus_adaptor( akonadicalendar_LIB_SRC ../interfaces/org.freedesktop.Akonadi.Resource.FreeBusyProvider.xml freebusyproviderbase_p.h Akonadi::FreeBusyProviderBasePrivate freebusyprovideradaptor  Akonadi__FreeBusyProviderAdaptor )
kde4_add_library(akonadi-calendar ${LIBRARY_TYPE} ${akonadicalendar_LIB_SRC})

target_link_libraries(akonadi-calendar
                      akonadi-kde
                      kpimutils
                      kcalcore
                      ${KDE4_KDEUI_LIBS}
                      ${KDE4_KIO_LIBS}
                      ${KDE4_PHONON_LIBS})

if(NOT WINCE)
target_link_libraries(akonadi-calendar 
                                      ${NEPOMUK_LIBRARIES})
endif(NOT WINCE)

set_target_properties(akonadi-calendar PROPERTIES VERSION ${GENERIC_LIB_VERSION} SOVERSION ${GENERIC_LIB_SOVERSION})

install(TARGETS akonadi-calendar EXPORT kdepimlibsLibraryTargets ${INSTALL_TARGETS_DEFAULT_ARGS})

install( FILES
  freebusyproviderbase.h
  standardcalendaractionmanager.h
  akonadi-calendar_export.h
  DESTINATION ${INCLUDE_INSTALL_DIR}/akonadi/calendar COMPONENT Devel
)
