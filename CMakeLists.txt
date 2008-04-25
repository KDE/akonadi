project(akonadi)

# where to look first for cmake modules, before ${CMAKE_ROOT}/Modules/ is checked
set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules")

set(LIB_SUFFIX "" CACHE STRING "Define suffix of directory name (32/64)" )
set(EXEC_INSTALL_PREFIX  ${CMAKE_INSTALL_PREFIX} CACHE PATH "Base directory for executables and libraries" FORCE)

## the following are directories where stuff will be installed to
set(BIN_INSTALL_DIR  "${EXEC_INSTALL_PREFIX}/bin" CACHE PATH "The binary install dir (default prefix/bin)" FORCE)
set(LIB_INSTALL_DIR "${EXEC_INSTALL_PREFIX}/lib${LIB_SUFFIX}" CACHE PATH "The subdirectory relative to the install prefix where libraries will be installed (default is /lib${LIB_SUFFIX})" FORCE)
set(INCLUDE_INSTALL_DIR "${CMAKE_INSTALL_PREFIX}/include" CACHE PATH "The subdirectory to the header prefix" FORCE)

set(QT_MIN_VERSION 4.4.0)

SET(AKONADI_LIB_MAJOR_VERSION "0")
SET(AKONADI_LIB_MINOR_VERSION "80")
SET(AKONADI_LIB_PATCH_VERSION "0")

SET(AKONADI_LIB_VERSION_STRING "${AKONADI_LIB_MAJOR_VERSION}.${AKONADI_LIB_MINOR_VERSION}.${AKONADI_LIB_PATCH_VERSION}")

find_package(Qt4 REQUIRED)
if (QT_QTDBUS_FOUND)
   include(AkonadiInternal)

   configure_file(akonadi-prefix.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/akonadi-prefix.h)
   if(NOT WIN32)
      configure_file(${CMAKE_CURRENT_SOURCE_DIR}/akonadi.pc.cmake ${CMAKE_CURRENT_BINARY_DIR}/akonadi.pc @ONLY)
   endif(NOT WIN32)

   INCLUDE(CheckIncludeFiles)
   check_include_files(execinfo.h HAVE_EXECINFO_H)
   check_include_files(unistd.h HAVE_UNISTD_H)
   configure_file(config-akonadi.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-akonadi.h)


   if (CMAKE_COMPILER_IS_GNUCXX OR CMAKE_C_COMPILER MATCHES "icc")
      set (_ENABLE_EXCEPTIONS -fexceptions)
      set ( CMAKE_C_FLAGS     "${CMAKE_C_FLAGS} -Wno-long-long -std=iso9899:1990 -Wundef -Wcast-align -Werror-implicit-function-declaration -Wchar-subscripts -Wall -W -Wpointer-arith -Wwrite-strings -Wformat-security -Wmissing-format-attribute -fno-common")
      set ( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wnon-virtual-dtor -Wno-long-long -ansi -Wundef -Wcast-align -Wchar-subscripts -Wall -W -Wpointer-arith -Wformat-security -fno-check-new -fno-common")
   else (CMAKE_COMPILER_IS_GNUCXX OR CMAKE_C_COMPILER MATCHES "icc")
      if (MSVC)
         set (_ENABLE_EXCEPTIONS -EHsc)
      endif (MSVC)
   endif (CMAKE_COMPILER_IS_GNUCXX OR CMAKE_C_COMPILER MATCHES "icc")

   add_subdirectory(cmake)

   include_directories(${CMAKE_SOURCE_DIR} ${CMAKE_BINARY_DIR} ${CMAKE_BINARY_DIR}/akonadi ${QT_INCLUDES})

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
      add_subdirectory(server)
   endif (XSLTPROC_EXECUTABLE)

   #### MIME type spezializations

   set(SHARED_MIME_INFO_MINIMUM_VERSION "0.20")
   find_package(SharedMimeInfo REQUIRED)

   ########### next target ###############

   install(FILES akonadi-mime.xml DESTINATION ${XDG_MIME_INSTALL_DIR})
   update_xdg_mimetypes(${XDG_MIME_INSTALL_DIR})

   if(NOT WIN32)
     install(FILES ${CMAKE_CURRENT_BINARY_DIR}/akonadi.pc
             DESTINATION ${LIB_INSTALL_DIR}/pkgconfig )
   endif(NOT WIN32)

else(QT_QTDBUS_FOUND)
   message(STATUS "Akonadi requires Qt4 with QtDBus module in order to built.")
endif(QT_QTDBUS_FOUND)
macro_display_feature_log()
