project(Akonadi)

cmake_minimum_required(VERSION 2.6.0 FATAL_ERROR)

# where to look first for cmake modules, before ${CMAKE_ROOT}/Modules/ is checked
set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules")


############### Build Options ###############

option(AKONADI_BUILD_TESTS "Build the Akonadi unit tests." TRUE)
if (AKONADI_BUILD_TESTS)
  enable_testing()
endif (AKONADI_BUILD_TESTS)


############### CMake Macros ###############

include(MacroLogFeature)
include(InstallSettings)
include(CheckIncludeFiles)


############### The Akonadi version (used in AkonadiConfig.cmake) ###############

set(AKONADI_VERSION_MAJOR "1")
set(AKONADI_VERSION_MINOR "1")
set(AKONADI_VERSION_PATCH "85")
set(AKONADI_VERSION "${AKONADI_VERSION_MAJOR}.${AKONADI_VERSION_MINOR}.${AKONADI_VERSION_PATCH}")

# If Subversion is installed, and a '.svn' directory is found,
# we append the SVN revision to AKONADI_VERSION_STRING.
set(AKONADI_VERSION_STRING "${AKONADI_VERSION}")
if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/.svn")
  if (NOT Subversion_FOUND)
    find_package( Subversion)
  endif (NOT Subversion_FOUND)
  if (Subversion_FOUND)
    Subversion_WC_INFO( ${PROJECT_SOURCE_DIR} Akonadi)
    set(AKONADI_VERSION_STRING "${AKONADI_VERSION_STRING} (revision ${Akonadi_WC_LAST_CHANGED_REV})")
  endif (Subversion_FOUND)
endif (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/.svn")


############### Find what we need ###############

#### Qt4 ####
set(QT_MIN_VERSION 4.5.0)
find_package(Qt4 REQUIRED)

# We need QtDBus and QtSql
if (NOT QT_QTDBUS_FOUND)
   message(FATAL_ERROR  "Akonadi requires Qt4 with QtDBus module in order to built.")
endif(NOT QT_QTDBUS_FOUND)
if (NOT QT_QTSQL_FOUND)
   message(FATAL_ERROR  "Akonadi requires Qt4 with QtSql module in order to built.")
endif(NOT QT_QTSQL_FOUND)
# properly set up compile flags (QT_DEBUG/QT_NO_DEBUG, ...)
include(${QT_USE_FILE})

#### Soprano ####
find_package(Soprano)

# There are different cases :
# 1/ Soprano is already installed on the system, we use it
# 2/ Soprano is not installed but we're actually building kdesupport
# 3/ Soprano wasn't found and we're not building kdesupport.
# Only the 3rd case should display an error.

if (NOT Soprano_FOUND)
  if("${KDESupport_SOURCE_DIR}" STREQUAL "${CMAKE_SOURCE_DIR}")
    message(STATUS "Soprano is needed to build Akonadi but is not installed yet. Let's use the source and build dirs instead.")
    set (SOPRANO_INCLUDE_DIR ${KDESupport_SOURCE_DIR}/soprano)
    set (SOPRANO_LIBRARIES ${KDESupport_BINARY_DIR}/soprano/soprano/libsoprano.so) # Doesn't exist yet.
    set (Soprano_FOUND TRUE)
  endif("${KDESupport_SOURCE_DIR}" STREQUAL "${CMAKE_SOURCE_DIR}")
endif (NOT Soprano_FOUND)

macro_log_feature(Soprano_FOUND "Soprano" "Semantic Desktop Storing" "" TRUE "" "Soprano is required to build Akonadi")

#### Automoc ####
find_package(Automoc4 REQUIRED)

#### SMI ####
set(SHARED_MIME_INFO_MINIMUM_VERSION "0.20")
find_package(SharedMimeInfo REQUIRED)

#### XSLTProc ####
find_program(XSLTPROC_EXECUTABLE xsltproc)
if(NOT XSLTPROC_EXECUTABLE)
  message(FATAL_ERROR "\nThe command line XSLT processor program 'xsltproc'  could not be found.\nPlease install xsltproc.\n")
endif(NOT XSLTPROC_EXECUTABLE)

#### Boost ####
find_package(Boost COMPONENTS program_options)
macro_log_feature(Boost_FOUND "Boost" "Boost C++ Libraries" "http://www.boost.org" TRUE "" "Akonadi requires the Boost C++ libraries.")

# should be handled by FindBoost.cmake -> cmake bug #8335
if(NOT Boost_USE_STATIC_LIBS)
  add_definitions(-DBOOST_DYN_LINK)
endif(NOT Boost_USE_STATIC_LIBS)


############### Compilers flags ###############

if(MSVC)
  # otherwise we get an undefined reference to
  # boost::program_options::options_description::m_default_line_length
  set(Boost_USE_STATIC_LIBS ON)
  set (_ENABLE_EXCEPTIONS -EHsc)
endif(MSVC)

if (CMAKE_COMPILER_IS_GNUCXX OR CMAKE_C_COMPILER MATCHES "icc")
  set (_ENABLE_EXCEPTIONS -fexceptions)
  set (CMAKE_C_FLAGS     "${CMAKE_C_FLAGS} -Wno-long-long -std=iso9899:1990 -Wundef -Wcast-align -Werror-implicit-function-declaration -Wchar-subscripts -Wall -W -Wpointer-arith -Wwrite-strings -Wformat-security -Wmissing-format-attribute -fno-common")
  set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wnon-virtual-dtor -Wno-long-long -ansi -Wundef -Wcast-align -Wchar-subscripts -Wall -W -Wpointer-arith -Wformat-security -fno-check-new -fno-common")
  if(CMAKE_COMPILE_GCOV)
    set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fprofile-arcs -ftest-coverage")
  endif(CMAKE_COMPILE_GCOV)
endif (CMAKE_COMPILER_IS_GNUCXX OR CMAKE_C_COMPILER MATCHES "icc")


############### Configure checks ###############

check_include_files(execinfo.h HAVE_EXECINFO_H)
check_include_files(unistd.h HAVE_UNISTD_H)

# set the output paths
set (EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin)
if (WIN32)
  set(LIBRARY_OUTPUT_PATH   ${EXECUTABLE_OUTPUT_PATH})
else (WIN32)
  set(LIBRARY_OUTPUT_PATH   ${CMAKE_BINARY_DIR}/lib)
endif (WIN32)

# Set up RPATH handling, so the libs are found if they are installed to a non-standard location.
# By default cmake builds the targets with full RPATH to everything in the build directory,
# but then removes the RPATH when installing.
# These two options below make it set the RPATH of the installed targets to all
# RPATH directories outside the current CMAKE_BINARY_DIR and also the library
# install directory. Alex
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
set(CMAKE_INSTALL_RPATH "${LIB_INSTALL_DIR}")

if(APPLE)
   set(CMAKE_INSTALL_NAME_DIR ${LIB_INSTALL_DIR})
endif(APPLE)


############### Macros ###############
macro(MAKE_INSTALL_PATH_ABSOLUTE out in)
   if (IS_ABSOLUTE "${in}")
      set(${out} "${in}")
   else (IS_ABSOLUTE "${in}")
      set(${out} "\${KDEPIMLIBS_INSTALL_DIR}/${in}")
   endif (IS_ABSOLUTE "${in}")
endmacro(MAKE_INSTALL_PATH_ABSOLUTE out in)


############### Generate files ###############

configure_file(akonadi-prefix.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/akonadi-prefix.h)
configure_file(config-akonadi.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-akonadi.h)
#configure_file(AkonadiConfig.cmake.in "${CMAKE_CURRENT_BINARY_DIR}/AkonadiConfig.cmake" @ONLY)

if (NOT WIN32)
  configure_file(${CMAKE_CURRENT_SOURCE_DIR}/akonadi.pc.cmake ${CMAKE_CURRENT_BINARY_DIR}/akonadi.pc @ONLY)
endif (NOT WIN32)


############### build targets ###############

include_directories(${Akonadi_SOURCE_DIR} ${Akonadi_BINARY_DIR} ${QT_INCLUDES} ${Boost_INCLUDE_DIR})

add_subdirectory(interfaces)
add_subdirectory(libs)

if (XSLTPROC_EXECUTABLE)
   add_subdirectory(server)
endif (XSLTPROC_EXECUTABLE)


############### install stuff ###############

install(FILES akonadi-mime.xml DESTINATION ${XDG_MIME_INSTALL_DIR})
update_xdg_mimetypes(${XDG_MIME_INSTALL_DIR})

if(NOT WIN32)
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/akonadi.pc DESTINATION ${LIB_INSTALL_DIR}/pkgconfig)
endif(NOT WIN32)

macro_display_feature_log()
