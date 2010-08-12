
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -UQT_NO_CAST_FROM_ASCII -UQT_NO_CAST_TO_ASCII ${KDE4_ENABLE_EXCEPTIONS}")

find_package(LibXslt)
macro_log_feature(XSLTPROC_EXECUTABLE "xsltproc" "The command line XSLT processor from libxslt" "http://xmlsoft.org/XSLT/" TRUE "" "Required to generate the D-Bus interfaces.")

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
endmacro(kcfg_generate_dbus_interface)

macro(add_resource_iface _kcfgFile _ifaceName _className)
  kcfg_generate_dbus_interface(${CMAKE_CURRENT_SOURCE_DIR}/${_kcfgFile} ${_ifaceName})
  string(TOLOWER ${_className} _codeFile)
  set_source_files_properties( ${CMAKE_CURRENT_BINARY_DIR}/${_ifaceName}.xml PROPERTIES INCLUDE "metatype.h")
  qt4_add_dbus_interface(akonadimail_LIB_SRC
    ${CMAKE_CURRENT_BINARY_DIR}/${_ifaceName}.xml ${_codeFile} ${_className}
  )
endmacro(add_resource_iface)

add_resource_iface( imapresource.kcfg org.kde.Akonadi.Imap.Settings ImapSettings )
#qt4_add_dbus_interface(akonadimail_LIB_SRC org.kde.Akonadi.Imap.Settings.xml imapsettings ImapSettings )

########### next target ###############

set(akonadimail_LIB_SRC ${akonadimail_LIB_SRC} commandbase.cpp util.cpp
  markascommand.cpp
  movecommand.cpp
  movetotrashcommand.cpp
  standardmailactionmanager.cpp
)
kde4_add_library(akonadi-mail ${LIBRARY_TYPE} ${akonadimail_LIB_SRC})

target_link_libraries(akonadi-mail    akonadi-kde
                                      akonadi-kmime
                                      kmime
                                      kpimutils
                                      ${KDE4_KDEUI_LIBS}
                                      ${KDE4_KIO_LIBS}
                                      ${KDE4_PHONON_LIBS}
                                      ${NEPOMUK_LIBRARIES})

set_target_properties(akonadi-mail PROPERTIES VERSION ${GENERIC_LIB_VERSION} SOVERSION ${GENERIC_LIB_SOVERSION})

install(TARGETS akonadi-mail EXPORT kdepimlibsLibraryTargets ${INSTALL_TARGETS_DEFAULT_ARGS})

install( FILES
  standardmailactionmanager.h
  akonadi-mail_export.h
  DESTINATION ${INCLUDE_INSTALL_DIR}/akonadi/mail COMPONENT Devel
)
