include_directories(
        ${CMAKE_SOURCE_DIR}/
        ${QT_QTDBUS_INCLUDE_DIR}
        ${Boost_INCLUDE_DIR}
        ${CMAKE_CURRENT_BINARY_DIR}/akonadi
)

add_subdirectory( tests )

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

add_definitions(
  -DAKONADI_DEPRECATED=
  -DAKONADI_KMIME_DEPRECATED=
)
# This one won't be needed when CMake 2.8.13 is depended on.
add_definitions(
  -DAKONADI_DEPRECATED_EXPORT=AKONADI_EXPORT
  -DAKONADI_KMIME_DEPRECATED_EXPORT=AKONADI_KMIME_EXPORT
)

########### next target ###############

set( kmimeakonadi_LIB_SRC 
  addressattribute.cpp
  attributeregistrar.cpp
  removeduplicatesjob.cpp
  specialmailcollections.cpp
  specialmailcollectionsrequestjob.cpp
  specialmailcollectionsdiscoveryjob.cpp
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

include_directories(
  ${CMAKE_BINARY_DIR}/kmime
)

add_library( akonadi-kmime ${LIBRARY_TYPE} ${kmimeakonadi_LIB_SRC} )
if(CMAKE_MINIMUM_REQUIRED_VERSION VERSION_LESS 2.8.12)
  set_property(TARGET akonadi-kmime PROPERTY DEFINE_SYMBOL akonadi_kmime)
endif()
generate_export_header(akonadi-kmime
  BASE_NAME akonadi_kmime
  EXPORT_FILE_NAME akonadi-kmime_export.h
)

if (KDE4_BUILD_TESTS)
   set(AKONADI_KMIME_TEST_EXPORT AKONADI_KMIME_EXPORT)
endif()
configure_file(akonadi-kmimeprivate_export.h.in
  "${CMAKE_CURRENT_BINARY_DIR}/akonadi-kmimeprivate_export.h"
)

target_link_libraries( akonadi-kmime akonadi-kde kmime ${QT_QTGUI_LIBRARY} ${KDE4_KDECORE_LIBS} ${KDE4_KDEUI_LIBS} ${KDE4_KIO_LIBS})
set_target_properties( akonadi-kmime PROPERTIES VERSION ${GENERIC_LIB_VERSION} SOVERSION ${GENERIC_LIB_SOVERSION} )
install(TARGETS akonadi-kmime EXPORT kdepimlibsLibraryTargets ${INSTALL_TARGETS_DEFAULT_ARGS})

########### install files ###############

install( FILES
  addressattribute.h
  removeduplicatesjob.h
  specialmailcollections.h
  specialmailcollectionsrequestjob.h
  specialmailcollectionsdiscoveryjob.h
  standardmailactionmanager.h
  ${CMAKE_CURRENT_BINARY_DIR}/akonadi-kmime_export.h
  messagefolderattribute.h
  messagemodel.h
  messageparts.h
  messageflags.h
  messagestatus.h
  messagethreadingattribute.h
  messagethreaderproxymodel.h
  DESTINATION ${INCLUDE_INSTALL_DIR}/akonadi/kmime COMPONENT Devel
)
