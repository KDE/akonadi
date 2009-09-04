include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
    ${Boost_INCLUDE_DIR}
    ${CMAKE_SOURCE_DIR}/kabc
)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DQT_NO_CAST_FROM_ASCII -DQT_NO_CAST_TO_ASCII ${KDE4_ENABLE_EXCEPTIONS}")

########### next target ###############

set(akonadicontact_editor_SRCS
  editor/addresseditwidget.cpp
  editor/contacteditorwidget.cpp
  editor/dateeditwidget.cpp
  editor/displaynameeditwidget.cpp
  editor/emaileditwidget.cpp
  editor/freebusyeditwidget.cpp
  editor/geoeditwidget.cpp
  editor/imagewidget.cpp
  editor/imeditwidget.cpp
  editor/kdatepickerpopup.cpp
  editor/nameeditwidget.cpp
  editor/phoneeditwidget.cpp
  editor/secrecyeditwidget.cpp
  editor/soundeditwidget.cpp
)

set(akonadicontact_LIB_SRC
  addressbookcombobox.cpp
  addressbookselectiondialog.cpp
  asyncselectionhandler.cpp
  attributeregistrar.cpp
  collectionfiltermodel.cpp
  contactcompletionmodel.cpp
  contacteditor.cpp
  contacteditordialog.cpp
  contactgroupeditor.cpp
  contactgroupeditordialog.cpp
  contactgroupexpandjob.cpp
  contactgrouplineedit.cpp
  contactgroupsearchjob.cpp
  contactgroupviewer.cpp
  contactgroupviewerdialog.cpp
  contactmetadata.cpp
  contactmetadataattribute.cpp
  contactsearchjob.cpp
  contactviewer.cpp
  contactviewerdialog.cpp
  waitingoverlay.cpp
  ${akonadicontact_editor_SRCS}
)

qt4_wrap_ui(akonadicontact_LIB_SRC contactgroupeditor.ui)

kde4_add_library(akonadi-contact SHARED ${akonadicontact_LIB_SRC})

target_link_libraries(akonadi-contact akonadi-kde
                                      kabc
                                      kcal
                                      ${KDE4_KDEUI_LIBS}
                                      ${KDE4_KIO_LIBS}
                                      ${KDE4_PHONON_LIBS})
target_link_libraries(akonadi-contact LINK_INTERFACE_LIBRARIES
                                      akonadi-kde
                                      kabc
                                      kcal
                                      ${KDE4_KDEUI_LIBS}
                                      ${KDE4_KIO_LIBS}
                                      ${KDE4_PHONON_LIBS})

set_target_properties(akonadi-contact PROPERTIES VERSION ${GENERIC_LIB_VERSION} SOVERSION ${GENERIC_LIB_SOVERSION})

install(TARGETS akonadi-contact EXPORT kdepimlibsLibraryTargets ${INSTALL_TARGETS_DEFAULT_ARGS})
install(FILES editor/pics/world.jpg DESTINATION ${DATA_INSTALL_DIR}/akonadi/contact/pics)
install(FILES editor/data/zone.tab DESTINATION ${DATA_INSTALL_DIR}/akonadi/contact/data)

install( FILES
  akonadi-contact_export.h
  addressbookselectiondialog.h
  contacteditor.h
  contacteditordialog.h
  contactgroupeditor.h
  contactgroupeditordialog.h
  contactgroupexpandjob.h
  contactgroupsearchjob.h
  contactgroupviewer.h
  contactgroupviewerdialog.h
  contactsearchjob.h
  contactviewer.h
  contactviewerdialog.h
  DESTINATION ${INCLUDE_INSTALL_DIR}/akonadi/contact COMPONENT Devel
)
