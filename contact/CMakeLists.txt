
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
    ${Boost_INCLUDE_DIR}
    ${CMAKE_SOURCE_DIR}/kabc
)

add_subdirectory(tests)
add_subdirectory(kcm)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DQT_NO_CAST_FROM_ASCII -DQT_NO_CAST_TO_ASCII ${KDE4_ENABLE_EXCEPTIONS}")

########### next target ###############

set(akonadicontact_actions_SRCS
  actions/dialphonenumberaction.cpp
  actions/showaddressaction.cpp
  actions/qskypedialer.cpp
)
kde4_add_kcfg_files(akonadicontact_actions_SRCS actions/contactactionssettings.kcfgc)

set(akonadicontact_editor_SRCS
  editor/addresseditwidget.cpp
  editor/categorieseditwidget.cpp
  editor/contacteditorwidget.cpp
  editor/dateeditwidget.cpp
  editor/displaynameeditwidget.cpp
  editor/emaileditwidget.cpp
  editor/freebusyeditwidget.cpp
  editor/geoeditwidget.cpp
  editor/imagewidget.cpp
  editor/imeditwidget.cpp
  editor/kdatepickerpopup.cpp
  editor/kedittagsdialog.cpp
  editor/nameeditdialog.cpp
  editor/nameeditwidget.cpp
  editor/phoneeditwidget.cpp
  editor/secrecyeditwidget.cpp
  editor/soundeditwidget.cpp
  editor/tagwidget.cpp
)

set(akonadicontact_LIB_SRC
  attributeregistrar.cpp
  collectionfiltermodel.cpp
  ../kdescendantsproxymodel.cpp
  contactcompletionmodel.cpp
  contactdefaultactions.cpp
  contacteditor.cpp
  contacteditordialog.cpp
  contactgroupeditor.cpp
  contactgroupeditordelegate.cpp
  contactgroupeditordialog.cpp
  contactgroupexpandjob.cpp
  contactgrouplineedit.cpp
  contactgroupmodel.cpp
  contactgroupsearchjob.cpp
  contactgroupviewer.cpp
  contactgroupviewerdialog.cpp
  contactmetadata.cpp
  contactmetadataattribute.cpp
  contactsearchjob.cpp
  contactviewer.cpp
  contactviewerdialog.cpp
  recentcontactscollections.cpp
  recentcontactscollectionrequestjob.cpp
  waitingoverlay.cpp
  ${akonadicontact_actions_SRCS}
  ${akonadicontact_editor_SRCS}
)

qt4_wrap_ui(akonadicontact_LIB_SRC contactgroupeditor.ui)
kde4_add_kcfg_files(akonadicontact_LIB_SRC recentcontactscollectionssettings.kcfgc )

kde4_add_library(akonadi-contact SHARED ${akonadicontact_LIB_SRC})

target_link_libraries(akonadi-contact akonadi-kde
                                      kabc
                                      kcal
                                      kpimutils
                                      ${KDE4_KDEUI_LIBS}
                                      ${KDE4_KIO_LIBS}
                                      ${KDE4_PHONON_LIBS}
                                      ${NEPOMUK_LIBRARIES})
target_link_libraries(akonadi-contact LINK_INTERFACE_LIBRARIES
                                      akonadi-kde
                                      kabc
                                      kcal
                                      kpimutils
                                      ${KDE4_KDEUI_LIBS}
                                      ${KDE4_KIO_LIBS}
                                      ${KDE4_PHONON_LIBS}
                                      ${NEPOMUK_LIBRARIES})

set_target_properties(akonadi-contact PROPERTIES VERSION ${GENERIC_LIB_VERSION} SOVERSION ${GENERIC_LIB_SOVERSION})

install(TARGETS akonadi-contact EXPORT kdepimlibsLibraryTargets ${INSTALL_TARGETS_DEFAULT_ARGS})
install(FILES editor/pics/world.jpg DESTINATION ${DATA_INSTALL_DIR}/akonadi/contact/pics)
install(FILES editor/data/zone.tab DESTINATION ${DATA_INSTALL_DIR}/akonadi/contact/data)
install(FILES recentcontactscollections.kcfg DESTINATION ${KCFG_INSTALL_DIR} )

install( FILES
  akonadi-contact_export.h
  contactdefaultactions.h
  contacteditor.h
  contacteditordialog.h
  contacteditorpageplugin.h
  contactgroupeditor.h
  contactgroupeditordialog.h
  contactgroupexpandjob.h
  contactgroupsearchjob.h
  contactgroupviewer.h
  contactgroupviewerdialog.h
  contactsearchjob.h
  contactviewer.h
  contactviewerdialog.h
  recentcontactscollectionrequestjob.h
  DESTINATION ${INCLUDE_INSTALL_DIR}/akonadi/contact COMPONENT Devel
)
