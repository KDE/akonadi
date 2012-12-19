
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
    ${Boost_INCLUDE_DIR}
    ${CMAKE_BINARY_DIR}/kabc
    ${CMAKE_SOURCE_DIR}/kabc
    ${NEPOMUK_CORE_INCLUDE_DIR}
)

if (Soprano_FOUND)
  include_directories(
      ${SOPRANO_INCLUDE_DIR}
  )
endif()

find_package(Prison QUIET)
set_package_properties(Prison PROPERTIES DESCRIPTION "Prison" URL "http://projects.kde.org/prison" TYPE OPTIONAL PURPOSE "Needed to show mobile barcodes of your contacts")
if (PRISON_FOUND)
    add_definitions(-DHAVE_PRISON)
    include_directories(${PRISON_INCLUDE_DIR})
endif()

if (WINCE)
  set( AKONADI_PHONE_DIAL_DEFAULT "UseWinCE" )
  set( AKONADI_SEND_SMS_DEFAULT "UseWinCESms" )
else()
  set( AKONADI_PHONE_DIAL_DEFAULT "UseSkype" )
  set( AKONADI_SEND_SMS_DEFAULT  "UseSkypeSms" )
endif()

add_subdirectory(tests)
#We don't have kcms under wince so don't build them
if(NOT WINCE)
  add_subdirectory(kcm)
endif()
add_subdirectory(editor/im)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DQT_NO_CAST_FROM_ASCII -DQT_NO_CAST_TO_ASCII ${KDE4_ENABLE_EXCEPTIONS}")

########### next target ###############

set(akonadicontact_actions_SRCS
  actions/dialphonenumberaction.cpp
  actions/showaddressaction.cpp
  actions/qdialer.cpp
  actions/qskypedialer.cpp
  actions/sendsmsaction.cpp
  actions/smsdialog.cpp
)

if (WINCE)
  set( akonadicontact_actions_SRCS ${akonadicontact_actions_SRCS}
    actions/qwincedialer.cpp
  )
endif()

configure_file( actions/contactactionssettings.kcfg.cmake ${CMAKE_CURRENT_BINARY_DIR}/contactactionssettings.kcfg @ONLY)
kde4_add_kcfg_files(akonadicontact_actions_SRCS actions/contactactionssettings.kcfgc)

set(akonadicontact_editor_SRCS
  editor/addresseditwidget.cpp
  editor/categorieseditwidget.cpp
  editor/contacteditorwidget.cpp
  editor/customfieldeditordialog.cpp
  editor/customfieldsdelegate.cpp
  editor/customfieldseditwidget.cpp
  editor/customfieldsmodel.cpp
  editor/dateeditwidget.cpp
  editor/displaynameeditwidget.cpp
  editor/emaileditwidget.cpp
  editor/freebusyeditwidget.cpp
  editor/geoeditwidget.cpp
  editor/im/imdelegate.cpp
  editor/im/imeditordialog.cpp
  editor/im/immodel.cpp
  editor/im/improtocols.cpp
  editor/im/imitemdialog.cpp
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
  abstractcontactformatter.cpp
  abstractcontactgroupformatter.cpp
  attributeregistrar.cpp
  collectionfiltermodel.cpp
  contactcompletionmodel.cpp
  contactdefaultactions.cpp
  contacteditor.cpp
  contacteditordialog.cpp
  contactgroupeditor.cpp
  contactgroupeditordelegate.cpp
  contactgroupeditordialog.cpp
  contactgrouplineedit.cpp
  contactgroupexpandjob.cpp
  contactgroupmodel.cpp
  contactgroupsearchjob.cpp
  contactgroupviewer.cpp
  contactgroupviewerdialog.cpp
  contactmetadata.cpp
  contactmetadataattribute.cpp
  contactsearchjob.cpp
  contactsfilterproxymodel.cpp
  contactstreemodel.cpp
  contactviewer.cpp
  contactviewerdialog.cpp
  customfields.cpp
  customfieldmanager.cpp
  emailaddressselection.cpp
  emailaddressselectiondialog.cpp
  emailaddressselectionproxymodel.cpp
  emailaddressselectionwidget.cpp
  textbrowser.cpp
  leafextensionproxymodel.cpp
  recentcontactscollections.cpp
  recentcontactscollectionrequestjob.cpp
  standardcontactactionmanager.cpp
  standardcontactformatter.cpp
  standardcontactgroupformatter.cpp
  waitingoverlay.cpp
  ${akonadicontact_actions_SRCS}
)

if(NOT KDE_PLATFORM_PROFILE STREQUAL "Mobile")
set(akonadicontact_LIB_SRC
    ${akonadicontact_LIB_SRC}
    ${akonadicontact_editor_SRCS}
)
else()
add_definitions(-DDISABLE_EDITOR_WIDGETS)
endif()

qt4_wrap_ui(akonadicontact_LIB_SRC contactgroupeditor.ui)
kde4_add_kcfg_files(akonadicontact_LIB_SRC recentcontactscollectionssettings.kcfgc )

kde4_add_library(akonadi-contact ${LIBRARY_TYPE} ${akonadicontact_LIB_SRC})

target_link_libraries(akonadi-contact akonadi-kde
                                      kabc
                                      kcalcore
                                      kmime
                                      kpimutils
                                      ${KDE4_KDEUI_LIBS}
				      ${KDE4_KIO_LIBS} 
                                      ${NEPOMUK_CORE_LIBRARY})

if(PRISON_FOUND) 
target_link_libraries(akonadi-contact ${PRISON_LIBRARIES})
endif()

target_link_libraries(akonadi-contact LINK_INTERFACE_LIBRARIES
                                      akonadi-kde
                                      kabc
                                      kcalcore
                                      kmime
                                      kpimutils
                                      ${KDE4_KDEUI_LIBS}
                                      ${KDE4_KIO_LIBS}
				      ${NEPOMUK_CORE_LIBRARY})

if (NOT WINCE)
target_link_libraries(akonadi-contact ${KDE4_PHONON_LIBS})
target_link_libraries(akonadi-contact LINK_INTERFACE_LIBRARIES ${KDE4_PHONON_LIBS})
endif()

set_target_properties(akonadi-contact PROPERTIES VERSION ${GENERIC_LIB_VERSION} SOVERSION ${GENERIC_LIB_SOVERSION})

install(TARGETS akonadi-contact EXPORT kdepimlibsLibraryTargets ${INSTALL_TARGETS_DEFAULT_ARGS})
install(FILES editor/pics/world.jpg DESTINATION ${DATA_INSTALL_DIR}/akonadi/contact/pics)
install(FILES editor/data/zone.tab DESTINATION ${DATA_INSTALL_DIR}/akonadi/contact/data)
install(FILES recentcontactscollections.kcfg DESTINATION ${KCFG_INSTALL_DIR} )

install( FILES
  abstractcontactformatter.h
  abstractcontactgroupformatter.h
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
  contactsfilterproxymodel.h
  contactstreemodel.h
  contactviewer.h
  contactviewerdialog.h
  emailaddressselection.h
  emailaddressselectiondialog.h
  emailaddressselectionwidget.h
  recentcontactscollectionrequestjob.h
  standardcontactactionmanager.h
  standardcontactformatter.h
  standardcontactgroupformatter.h
  DESTINATION ${INCLUDE_INSTALL_DIR}/akonadi/contact COMPONENT Devel
)
