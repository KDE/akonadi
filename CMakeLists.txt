project(akonadi)

# where to look first for cmake modules, before ${CMAKE_ROOT}/Modules/ is checked
set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/akonadi/cmake/modules")

find_package(Qt4 REQUIRED)

include(AkonadiInternal)

add_subdirectory(cmake)

configure_file(kdemacros.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/kdemacros.h )
configure_file(akonadi-prefix.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/akonadi-prefix.h)

INCLUDE(CheckIncludeFiles)
check_include_files(execinfo.h HAVE_EXECINFO_H)
configure_file(config-akonadi.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-akonadi.h)

include_directories(${CMAKE_SOURCE_DIR} ${CMAKE_BINARY_DIR} ${QT_INCLUDES})

# we need the absolute directories where stuff will be installed too
# but since the variables which contain the destinations can be relative
# or absolute paths, we need this macro to make them all absoulte, Alex
macro(MAKE_INSTALL_PATH_ABSOLUTE out in)
   if (UNIX)
      if ("${in}" MATCHES "^/.*")
         set(${out} "${in}")
      else ("${in}" MATCHES "^/.*")
         set(${out} "\${KDE4_INSTALL_DIR}/${in}")
      endif ("${in}" MATCHES "^/.*")
   else (UNIX)
      if ("${in}" MATCHES "^[a-zA-Z]:.*")
        set(${out} "${in}")
      else ("${in}" MATCHES "^[a-zA-Z]:.*")
         set(${out} "\${KDE4_INSTALL_DIR}/${in}")
      endif ("${in}" MATCHES "^[a-zA-Z]:.*")
   endif (UNIX)
endmacro(MAKE_INSTALL_PATH_ABSOLUTE out in)

make_install_path_absolute(AKONADI_DBUS_INTERFACES_DIR ${DBUS_INTERFACES_INSTALL_DIR})
make_install_path_absolute(AKONADI_DBUS_SERVICES_DIR   ${DBUS_SERVICES_INSTALL_DIR})

set(AKONADI_DBUS_INTERFACES_DIR \"${AKONADI_DBUS_INTERFACES_DIR}\")
set(AKONADI_DBUS_SERVICES_DIR   \"${AKONADI_DBUS_SERVICES_DIR}\")

add_subdirectory(interfaces)
add_subdirectory(libs)

find_package(LibXslt)
macro_log_feature(XSLTPROC_EXECUTABLE "xsltproc" "xsltproc" "http://xmlsoft.org/XSLT/" FALSE "" "Needed to generate Akonadi database code")

if (XSLTPROC_EXECUTABLE)
   #generates a D-Bus interface description from a KConfigXT file
   macro( kcfg_generate_dbus_interface _kcfg _name )
      add_custom_command(
         OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${_name}.xml
         COMMAND ${XSLTPROC_EXECUTABLE} --stringparam interfaceName ${_name}
         ${CMAKE_INSTALL_PREFIX}/share/apps/akonadi-kde/kcfg2dbus.xsl
         ${_kcfg}
         > ${CMAKE_CURRENT_BINARY_DIR}/${_name}.xml
         DEPENDS ${CMAKE_INSTALL_PREFIX}/share/apps/akonadi-kde/kcfg2dbus.xsl
         ${_kcfg}
         )
   endmacro( kcfg_generate_dbus_interface )
   add_subdirectory(server)
endif (XSLTPROC_EXECUTABLE)

#### MIME type spezializations

set(SHARED_MIME_INFO_MINIMUM_VERSION "0.20")
find_package(SharedMimeInfo REQUIRED)

########### next target ###############

install(FILES akonadi-mime.xml DESTINATION ${XDG_MIME_INSTALL_DIR})
update_xdg_mimetypes(${XDG_MIME_INSTALL_DIR})
