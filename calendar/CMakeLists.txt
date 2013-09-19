
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
)

add_subdirectory( tests )

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -UQT_NO_CAST_FROM_ASCII -UQT_NO_CAST_TO_ASCII ${KDE4_ENABLE_EXCEPTIONS}")

set(akonadicalendar_LIB_SRC
  blockalarmsattribute.cpp
  calendarbase.cpp
  calendarclipboard.cpp
  calendarmodel.cpp
  calfilterproxymodel_p.cpp
  etmcalendar.cpp
  history.cpp
  history_p.cpp
  icalimporter.cpp
  incidencefetchjob_p.cpp
  incidencechanger.cpp
  itiphandler.cpp
  itiphandler_p.cpp
  itiphandlerhelper_p.cpp
  kcolumnfilterproxymodel.cpp
  fetchjobcalendar.cpp
  freebusydownloadjob_p.cpp
  freebusymanager.cpp
  freebusyproviderbase.cpp
  mailclient_p.cpp
  mailscheduler_p.cpp
  publishdialog.cpp
  publishdialog_p.cpp
  scheduler_p.cpp
  standardcalendaractionmanager.cpp
  todopurger.cpp
  utils_p.cpp
)

kde4_add_kcfg_files(akonadicalendar_LIB_SRC calendarsettings.kcfgc)
kde4_add_ui_files(akonadicalendar_LIB_SRC publishdialog_base.ui)

qt4_add_dbus_adaptor( akonadicalendar_LIB_SRC ../interfaces/org.freedesktop.Akonadi.Resource.FreeBusyProvider.xml freebusyproviderbase_p.h Akonadi::FreeBusyProviderBasePrivate freebusyprovideradaptor  Akonadi__FreeBusyProviderAdaptor )
kde4_add_library(akonadi-calendar ${LIBRARY_TYPE} ${akonadicalendar_LIB_SRC})

target_link_libraries(akonadi-calendar
                      akonadi-contact
                      akonadi-kmime
                      akonadi-kde
                      kpimidentities
                      kpimutils
                      kcalcore
                      kcalutils
                      kmime
                      mailtransport
                      ${KDE4_KDEUI_LIBS}
                      ${KDE4_KIO_LIBS}
                      ${KDE4_PHONON_LIBS})

set_target_properties(akonadi-calendar PROPERTIES VERSION ${GENERIC_LIB_VERSION} SOVERSION ${GENERIC_LIB_SOVERSION})

install(TARGETS akonadi-calendar EXPORT kdepimlibsLibraryTargets ${INSTALL_TARGETS_DEFAULT_ARGS})

install( FILES
  akonadi-calendar_export.h
  blockalarmsattribute.h
  calendarbase.h
  calendarclipboard.h
  etmcalendar.h
  history.h
  icalimporter.h
  incidencechanger.h
  itiphandler.h
  fetchjobcalendar.h
  freebusymanager.h
  freebusyproviderbase.h
  publishdialog.h
  standardcalendaractionmanager.h
  todopurger.h
  ${CMAKE_CURRENT_BINARY_DIR}/calendarsettings.h
  DESTINATION ${INCLUDE_INSTALL_DIR}/akonadi/calendar COMPONENT Devel
)
