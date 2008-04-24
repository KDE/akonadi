project(akonadi)

# where to look first for cmake modules, before ${CMAKE_ROOT}/Modules/ is checked
set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules")

set( QT_MIN_VERSION 4.4.0)

find_package(Qt4 REQUIRED)
if (QT_QTDBUS_FOUND)
   include(AkonadiInternal)

   add_subdirectory(cmake)

   configure_file(akonadi-prefix.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/akonadi-prefix.h)

   INCLUDE(CheckIncludeFiles)
   check_include_files(execinfo.h HAVE_EXECINFO_H)
   check_include_files(unistd.h HAVE_UNISTD_H)
   configure_file(config-akonadi.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-akonadi.h)


   if (CMAKE_COMPILER_IS_GNUCXX OR CMAKE_C_COMPILER MATCHES "icc")
      set (_ENABLE_EXCEPTIONS -fexceptions)
   else (CMAKE_COMPILER_IS_GNUCXX OR CMAKE_C_COMPILER MATCHES "icc")
      if (MSVC)
         set (_ENABLE_EXCEPTIONS -EHsc)
      endif (MSVC)
   endif (CMAKE_COMPILER_IS_GNUCXX OR CMAKE_C_COMPILER MATCHES "icc")

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
else(QT_QTDBUS_FOUND)
   message(STATUS "Akonadi requires Qt4 with QtDBus module in order to built.")
endif(QT_QTDBUS_FOUND)
