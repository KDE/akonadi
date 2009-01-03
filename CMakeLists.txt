project(Akonadi)

cmake_minimum_required(VERSION 2.4.5 FATAL_ERROR)

# enable unit tests
enable_testing()

# where to look first for cmake modules, before ${CMAKE_ROOT}/Modules/ is checked
set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules")

######## find packages ####################

include(MacroLogFeature)

set(QT_MIN_VERSION 4.4.0)
find_package(Qt4 REQUIRED)
# properly set up compile flags (QT_DEBUG/QT_NO_DEBUG, ...)
include(${QT_USE_FILE})
if (NOT QT_QTDBUS_FOUND)
   message(FATAL_ERROR  "Akonadi requires Qt4 with QtDBus module in order to built.")
endif(NOT QT_QTDBUS_FOUND)

find_package(Automoc4 REQUIRED)

set(SHARED_MIME_INFO_MINIMUM_VERSION "0.20")
find_package(SharedMimeInfo REQUIRED)

find_program(XSLTPROC_EXECUTABLE xsltproc)
if(NOT XSLTPROC_EXECUTABLE)
  message(FATAL_ERROR "\nThe command line XSLT processor program 'xsltproc'  could not be found.\nPlease install xsltproc.\n")
endif(NOT XSLTPROC_EXECUTABLE)

if(MSVC)
  # otherwise we get an undefined reference to
  # boost::program_options::options_description::m_default_line_length
  set(Boost_USE_STATIC_LIBS ON)
endif(MSVC)
find_package(Boost REQUIRED COMPONENTS program_options)
if(NOT Boost_FOUND)
  message(FATAL_ERROR "Akonadi requires the Boost C++ libraries.")
endif(NOT Boost_FOUND)
include_directories(${Boost_INCLUDE_DIR})
# should be handled by FindBoost.cmake -> cmake bug #8335
if(NOT Boost_USE_STATIC_LIBS)
  add_definitions(-DBOOST_DYN_LINK)
endif(NOT Boost_USE_STATIC_LIBS)

# this one actually sets only install locations
include(InstallSettings)

################ configure checks and similar things ##############################

set(AKONADI_LIB_MAJOR_VERSION "1")
set(AKONADI_LIB_MINOR_VERSION "1")
set(AKONADI_LIB_PATCH_VERSION "0")
set(AKONADI_LIB_VERSION_STRING "${AKONADI_LIB_MAJOR_VERSION}.${AKONADI_LIB_MINOR_VERSION}.${AKONADI_LIB_PATCH_VERSION}")

set( AKONADI_VERSION_STRING "${AKONADI_LIB_VERSION_STRING}" )
if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/.svn")
  if ( NOT Subversion_FOUND )
    find_package( Subversion )
  endif ( NOT Subversion_FOUND )
  if ( Subversion_FOUND )
    Subversion_WC_INFO( ${PROJECT_SOURCE_DIR} Akonadi )
    set( AKONADI_VERSION_STRING "${AKONADI_LIB_VERSION_STRING} (revision ${Akonadi_WC_LAST_CHANGED_REV})" )
  endif ( Subversion_FOUND )
endif (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/.svn")

# set the output paths
set (EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin)
if (WIN32)
   set(LIBRARY_OUTPUT_PATH            ${EXECUTABLE_OUTPUT_PATH} )
else (WIN32)
   set(LIBRARY_OUTPUT_PATH            ${CMAKE_BINARY_DIR}/lib )
endif (WIN32)


# Set up RPATH handling, so the libs are found if they are installed to a non-standard location.
# By default cmake builds the targets with full RPATH to everything in the build directory,
# but then removes the RPATH when installing.
# These two options below make it set the RPATH of the installed targets to all
# RPATH directories outside the current CMAKE_BINARY_DIR and also the library
# install directory. Alex
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH  TRUE)
set(CMAKE_INSTALL_RPATH                "${LIB_INSTALL_DIR}" )

if(APPLE)
   set(CMAKE_INSTALL_NAME_DIR ${LIB_INSTALL_DIR})
endif(APPLE)


configure_file(akonadi-prefix.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/akonadi-prefix.h)

include(CheckIncludeFiles)
check_include_files(execinfo.h HAVE_EXECINFO_H)
check_include_files(unistd.h HAVE_UNISTD_H)
configure_file(config-akonadi.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-akonadi.h)


# maybe the handling for the different compilers could be split out into a separate file, Alex
if (CMAKE_COMPILER_IS_GNUCXX OR CMAKE_C_COMPILER MATCHES "icc")
   set (_ENABLE_EXCEPTIONS -fexceptions)
   set ( CMAKE_C_FLAGS     "${CMAKE_C_FLAGS} -Wno-long-long -std=iso9899:1990 -Wundef -Wcast-align -Werror-implicit-function-declaration -Wchar-subscripts -Wall -W -Wpointer-arith -Wwrite-strings -Wformat-security -Wmissing-format-attribute -fno-common")
   set ( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wnon-virtual-dtor -Wno-long-long -ansi -Wundef -Wcast-align -Wchar-subscripts -Wall -W -Wpointer-arith -Wformat-security -fno-check-new -fno-common")
else (CMAKE_COMPILER_IS_GNUCXX OR CMAKE_C_COMPILER MATCHES "icc")
   if (MSVC)
      set (_ENABLE_EXCEPTIONS -EHsc)
   endif (MSVC)
endif (CMAKE_COMPILER_IS_GNUCXX OR CMAKE_C_COMPILER MATCHES "icc")


################# build targets ########################

# Use the project specific source and binary directories, they are always the same
# both when building akonadi as part of kdesupport or separate CMAKE_(SOURCE|BINARY)_DIR
# are the root directory of the cmake build tree, i.e. kdesupport/ if akonadi is built
# as part o kdesupport and kdesupport/akonadi/ if it is build separate.
# (CMAKE_CURRENT_(SOURCE|BINARY)_DIR are also always the same). Alex
include_directories(${Akonadi_SOURCE_DIR} ${Akonadi_BINARY_DIR} ${QT_INCLUDES})

# if something breaks because the block below is commented out,
# please send me an email, Alex <neundorf@kde.org>

# # we need the absolute directories where stuff will be installed too
# # but since the variables which contain the destinations can be relative
# # or absolute paths, we need this macro to make them all absoulte, Alex
# macro(MAKE_INSTALL_PATH_ABSOLUTE out in)
#    if (UNIX)
#       if ("${in}" MATCHES "^/.*")
#          set(${out} "${in}")
#       else ("${in}" MATCHES "^/.*")
#          set(${out} "\${KDE4_INSTALL_DIR}/${in}")
#       endif ("${in}" MATCHES "^/.*")
#    else (UNIX)
#       if ("${in}" MATCHES "^[a-zA-Z]:.*")
#          set(${out} "${in}")
#       else ("${in}" MATCHES "^[a-zA-Z]:.*")
#          set(${out} "\${KDE4_INSTALL_DIR}/${in}")
#       endif ("${in}" MATCHES "^[a-zA-Z]:.*")
#    endif (UNIX)
# endmacro(MAKE_INSTALL_PATH_ABSOLUTE out in)
#
# make_install_path_absolute(AKONADI_DBUS_INTERFACES_DIR ${DBUS_INTERFACES_INSTALL_DIR})
# make_install_path_absolute(AKONADI_DBUS_SERVICES_DIR   ${DBUS_SERVICES_INSTALL_DIR})
#
# set(AKONADI_DBUS_INTERFACES_DIR \"${AKONADI_DBUS_INTERFACES_DIR}\")
# set(AKONADI_DBUS_SERVICES_DIR   \"${AKONADI_DBUS_SERVICES_DIR}\")

add_subdirectory(interfaces)
add_subdirectory(libs)

if (XSLTPROC_EXECUTABLE)
   add_subdirectory(server)
endif (XSLTPROC_EXECUTABLE)

############# install stuff ######################

install(FILES akonadi-mime.xml DESTINATION ${XDG_MIME_INSTALL_DIR})
update_xdg_mimetypes(${XDG_MIME_INSTALL_DIR})

if(NOT WIN32)   # pkgconfig file
   configure_file(${CMAKE_CURRENT_SOURCE_DIR}/akonadi.pc.cmake ${CMAKE_CURRENT_BINARY_DIR}/akonadi.pc @ONLY)
   install(FILES ${CMAKE_CURRENT_BINARY_DIR}/akonadi.pc DESTINATION ${LIB_INSTALL_DIR}/pkgconfig )
endif(NOT WIN32)


macro_display_feature_log()
