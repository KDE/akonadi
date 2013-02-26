include_directories(
        ${CMAKE_SOURCE_DIR}/
        ${QT_QTDBUS_INCLUDE_DIR}
        ${Boost_INCLUDE_DIR}
)

# Qt5: Port
# add_subdirectory( tests )

set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DQT_NO_CAST_FROM_ASCII -DQT_NO_CAST_TO_ASCII ${KDE4_ENABLE_EXCEPTIONS}" )

find_package(LibXslt)
set_package_properties(LibXslt PROPERTIES DESCRIPTION "xsltproc" URL "http://xmlsoft.org/XSLT/" TYPE REQUIRED PURPOSE "Needed to generate D-Bus interface specifications")

# generates a D-Bus interface description from a KConfigXT file
macro(kcfg_generate_dbus_interface _kcfg _name)
  add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${_name}.xml
    COMMAND ${XSLTPROC_EXECUTABLE} --stringparam interfaceName ${_name}
    ${CMAKE_SOURCE_DIR}/akonadi/kcfg2dbus.xsl
    ${_kcfg}
    > ${CMAKE_CURRENT_BINARY_DIR}/${_name}.xml
    DEPENDS ${CMAKE_SOURCE_DIR}/akonadi/kcfg2dbus.xsl
    ${_kcfg}
    )
endmacro()

macro(add_resource_iface _kcfgFile _ifaceName _className)
  kcfg_generate_dbus_interface(${CMAKE_CURRENT_SOURCE_DIR}/${_kcfgFile} ${_ifaceName})
  string(TOLOWER ${_className} _codeFile)
  set_source_files_properties( ${CMAKE_CURRENT_BINARY_DIR}/${_ifaceName}.xml PROPERTIES INCLUDE "metatype.h")
  qt4_add_dbus_interface(kmimeakonadi_LIB_SRC
    ${CMAKE_CURRENT_BINARY_DIR}/${_ifaceName}.xml ${_codeFile} ${_className}
  )
endmacro()

########### next target ###############

set( kmimeakonadi_LIB_SRC 
  addressattribute.cpp
  attributeregistrar.cpp
  removeduplicatesjob.cpp
  specialmailcollections.cpp
  specialmailcollectionsrequestjob.cpp
  messagefolderattribute.cpp
  messagemodel.cpp
  messageparts.cpp
  messageflags.cpp
  messagestatus.cpp
  messagethreadingattribute.cpp
  messagethreaderproxymodel.cpp

  commandbase.cpp
  util.cpp
  emptytrashcommand.cpp
  markascommand.cpp
  movecommand.cpp
  movetotrashcommand.cpp
  standardmailactionmanager.cpp
)

add_resource_iface( imapresource.kcfg org.kde.Akonadi.Imap.Settings ImapSettings )

if (KDE4_BUILD_TESTS)
set( kmimeakonadi_LIB_SRC
  ${kmimeakonadi_LIB_SRC}
  specialmailcollectionstesting.cpp
)
endif()

kde4_add_kcfg_files( kmimeakonadi_LIB_SRC specialmailcollectionssettings.kcfgc )
install( FILES specialmailcollections.kcfg DESTINATION ${KCFG_INSTALL_DIR} )

kde4_add_library( akonadi-kmime ${LIBRARY_TYPE} ${kmimeakonadi_LIB_SRC} )

target_link_libraries( akonadi-kmime akonadi-kde kmime ${QT_QTGUI_LIBRARY} ${KDE4_KDECORE_LIBS} ${KDE4_KDEUI_LIBS} ${KDE4_KIO_LIBS} ${KDE4_KDE4SUPPORT_LIBS})
set_target_properties( akonadi-kmime PROPERTIES VERSION ${GENERIC_LIB_VERSION} SOVERSION ${GENERIC_LIB_SOVERSION} )
install(TARGETS akonadi-kmime EXPORT kdepimlibsLibraryTargets ${INSTALL_TARGETS_DEFAULT_ARGS})

########### install files ###############

install( FILES
  addressattribute.h
  removeduplicatesjob.h
  specialmailcollections.h
  specialmailcollectionsrequestjob.h
  standardmailactionmanager.h
  akonadi-kmime_export.h
  messagefolderattribute.h
  messagemodel.h
  messageparts.h
  messageflags.h
  messagestatus.h
  messagethreadingattribute.h
  messagethreaderproxymodel.h
  DESTINATION ${INCLUDE_INSTALL_DIR}/akonadi/kmime COMPONENT Devel
)
