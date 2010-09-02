project(Akonadi)

cmake_minimum_required(VERSION 2.6.2 FATAL_ERROR)

# where to look first for cmake modules, before ${CMAKE_ROOT}/Modules/ is checked
set(CMAKE_MODULE_PATH "${Akonadi_SOURCE_DIR}/cmake/modules")

############### Build Options ###############

include(CTest)
include(CTestConfig.cmake)
option(AKONADI_BUILD_TESTS "Build the Akonadi unit tests." TRUE)
option(INSTALL_QSQLITE_IN_QT_PREFIX "Install the QSQLite plugin in QT_PLUGIN_DIR" FALSE)
option(AKONADI_AGENT_SERVER "Build the experimental agent server" FALSE)

if (AKONADI_BUILD_TESTS)
  enable_testing()
endif (AKONADI_BUILD_TESTS)

if(${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION}.${CMAKE_PATCH_VERSION} VERSION_GREATER 2.6.2)
  option(USE_COMMON_CMAKE_PACKAGE_CONFIG_DIR "Prefer to install the <package>Config.cmake files to lib/cmake/<package> instead of lib/<package>/cmake" TRUE)
endif(${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION}.${CMAKE_PATCH_VERSION} VERSION_GREATER 2.6.2)

if (CMAKE_COMPILER_IS_GNUCXX)
   set(CMAKE_CXX_FLAGS_DEBUGFULL      "-g3 -fno-inline")
   set(CMAKE_C_FLAGS_DEBUGFULL        "-g3 -fno-inline")
endif()

############### CMake Macros ###############

include(MacroLogFeature)
include(InstallSettings)
include(CheckFunctionExists)
include(CheckIncludeFiles)
include(MacroWriteBasicCMakeVersionFile)

############### CTest options ###############
# Set a timeout value of 1 minute per test
set(DART_TESTING_TIMEOUT 60)

# CTestCustom.cmake has to be in the CTEST_BINARY_DIR.
# in the KDE build system, this is the same as CMAKE_BINARY_DIR.
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/CTestCustom.cmake ${CMAKE_CURRENT_BINARY_DIR}/CTestCustom.cmake)

############### The Akonadi version (used in AkonadiConfig.cmake) ###############

set(AKONADI_VERSION_MAJOR "1")
set(AKONADI_VERSION_MINOR "4")
set(AKONADI_VERSION_PATCH "52")
set(AKONADI_VERSION "${AKONADI_VERSION_MAJOR}.${AKONADI_VERSION_MINOR}.${AKONADI_VERSION_PATCH}")

# If Subversion is installed, and a '.svn' directory is found,
# we append the SVN revision to AKONADI_VERSION_STRING.
set(AKONADI_VERSION_STRING "${AKONADI_VERSION}")
if (EXISTS "${Akonadi_SOURCE_DIR}/.svn")
  if (NOT Subversion_FOUND)
    find_package( Subversion)
  endif (NOT Subversion_FOUND)
  if (Subversion_FOUND)
    Subversion_WC_INFO( ${PROJECT_SOURCE_DIR} Akonadi)
    set(AKONADI_VERSION_STRING "${AKONADI_VERSION_STRING} (revision ${Akonadi_WC_LAST_CHANGED_REV})")
  endif (Subversion_FOUND)
endif (EXISTS "${Akonadi_SOURCE_DIR}/.svn")


############### Find what we need ###############

#### Qt4 ####
set(QT_MIN_VERSION 4.6.0)
find_package(Qt4 REQUIRED)

# We need QtDBus and QtSql
set(QT_USE_QTDBUS TRUE)
set(QT_USE_QTSQL TRUE)
if (NOT QT_QTDBUS_FOUND)
   message(FATAL_ERROR  "Akonadi requires Qt4 with QtDBus module in order to built.")
endif(NOT QT_QTDBUS_FOUND)
if (NOT QT_QTSQL_FOUND)
   message(FATAL_ERROR  "Akonadi requires Qt4 with QtSql module in order to built.")
endif(NOT QT_QTSQL_FOUND)
# properly set up compile flags (QT_DEBUG/QT_NO_DEBUG, ...)
include(${QT_USE_FILE})

if(WINCE)
    find_package(Wcecompat REQUIRED)
    SET(QT_DBUSCPP2XML_EXECUTABLE ${HOST_BINDIR}/qdbuscpp2xml.exe)
    SET(QT_DBUSXML2CPP_EXECUTABLE ${HOST_BINDIR}/qdbusxml2cpp.exe)
    set(LIBRARY_TYPE STATIC)
    set(AKONADI_STATIC_LIBS ON)
    # still needed ?
    add_definitions(-DSQLITE3_STATIC_LIBS)
else(WINCE)
    set(LIBRARY_TYPE SHARED)
endif(WINCE)

#### Soprano ####

# There are different cases :
# 1/ We're running CMake in kdesupport. The Soprano dir exists and its compilation is enabled.
# 2/ We're running CMake in kdesupport but Soprano won't be built. we look for installed headers.
# 3/ We're just building akonadi. We look for the Soprano headers.

if("${KDESupport_SOURCE_DIR}" STREQUAL "${CMAKE_SOURCE_DIR}") # We ran CMake in kdesupport
  if(EXISTS ${KDESupport_SOURCE_DIR}/soprano AND BUILD_soprano) # and we plan to build Soprano
    message(STATUS "Soprano is needed to build Akonadi. We will use Soprano from kdesupport.")
    set (SOPRANO_INCLUDE_DIR ${KDESupport_SOURCE_DIR}/soprano ${KDESupport_SOURCE_DIR}/soprano/soprano ${KDESupport_BINARY_DIR}/soprano)
    set (SOPRANO_LIBRARIES soprano)
    set (Soprano_FOUND TRUE)
  endif(EXISTS ${KDESupport_SOURCE_DIR}/soprano AND BUILD_soprano)
endif("${KDESupport_SOURCE_DIR}" STREQUAL "${CMAKE_SOURCE_DIR}")

if(NOT Soprano_FOUND)
  find_package(Soprano)
endif(NOT Soprano_FOUND)

macro_log_feature(Soprano_FOUND "Soprano" "Semantic Desktop Storing" "http://soprano.sourceforge.net" TRUE "" "Soprano is required to build Akonadi")

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
if(WINCE)
set(XSLTPROC_EXECUTABLE "${HOST_BINDIR}/xsltproc.exe")
endif(WINCE)

#### Boost ####
if(MSVC)
  # otherwise we get an undefined reference to
  # boost::program_options::options_description::m_default_line_length
  set(Boost_USE_STATIC_LIBS ON)
  set (_ENABLE_EXCEPTIONS -EHsc)
endif(MSVC)

if(NOT WINCE)
    find_package(Boost COMPONENTS program_options)
    macro_log_feature(Boost_FOUND "Boost" "Boost C++ Libraries" "http://www.boost.org" TRUE "" "Akonadi requires the Boost C++ libraries.")
endif(NOT WINCE)

# should be handled by FindBoost.cmake -> cmake bug #8335
if(NOT Boost_USE_STATIC_LIBS)
  add_definitions(-DBOOST_DYN_LINK)
endif(NOT Boost_USE_STATIC_LIBS)

#### Sqlite ####
set(SQLITE_MIN_VERSION 3.6.23)
find_package(Sqlite)
macro_log_feature(SQLITE_FOUND "Sqlite" "Sqlite database library" "http://www.sqlite.org" FALSE "${SQLITE_MIN_VERSION}" "")

############### Compilers flags ###############

option(CMAKE_COMPILE_GCOV "Build with coverage support." FALSE)
if (CMAKE_COMPILER_IS_GNUCXX OR CMAKE_C_COMPILER MATCHES "icc")
  set (_ENABLE_EXCEPTIONS -fexceptions)
  set (CMAKE_C_FLAGS     "${CMAKE_C_FLAGS} -Wno-long-long -std=iso9899:1990 -Wundef -Wcast-align -Werror-implicit-function-declaration -Wchar-subscripts -Wall -W -Wpointer-arith -Wwrite-strings -Wformat-security -Wmissing-format-attribute -fno-common")
  set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wnon-virtual-dtor -Wno-long-long -ansi -Wundef -Wcast-align -Wchar-subscripts -Wall -W -Wpointer-arith -Wformat-security -fno-check-new -fno-common")
  if(CMAKE_COMPILE_GCOV)
    set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fprofile-arcs -ftest-coverage")
  endif(CMAKE_COMPILE_GCOV)
endif (CMAKE_COMPILER_IS_GNUCXX OR CMAKE_C_COMPILER MATCHES "icc")


############### Configure checks ###############

set (AKONADI_SYSTEM_LIBS)

check_include_files(execinfo.h HAVE_EXECINFO_H)
if (HAVE_EXECINFO_H)
  check_function_exists(backtrace BACKTRACE_IN_LIBC)
  if (NOT BACKTRACE_IN_LIBC)
    find_library (EXECINFO_LIBRARY NAMES execinfo)
    if (EXECINFO_LIBRARY)
      set (AKONADI_SYSTEM_LIBS ${AKONADI_SYSTEM_LIBS} ${EXECINFO_LIBRARY})
    endif (EXECINFO_LIBRARY)
  endif (NOT BACKTRACE_IN_LIBC)
endif (HAVE_EXECINFO_H)

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
# install directory, but only if this directory is not a default system library directory. Alex
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

list(FIND CMAKE_PLATFORM_IMPLICIT_LINK_DIRECTORIES "${LIB_INSTALL_DIR}" _isSystemLibDir)
if("${_isSystemLibDir}" STREQUAL "-1")
  set(CMAKE_INSTALL_RPATH "${LIB_INSTALL_DIR}")
endif("${_isSystemLibDir}" STREQUAL "-1")

if(APPLE)
   set(CMAKE_INSTALL_NAME_DIR ${LIB_INSTALL_DIR})
endif(APPLE)


############### Macros ###############

macro(MAKE_INSTALL_PATH_ABSOLUTE out in)
   if (IS_ABSOLUTE "${in}")
      set(${out} "${in}")
   else (IS_ABSOLUTE "${in}")
      set(${out} "\${AKONADI_INSTALL_DIR}/${in}")
   endif (IS_ABSOLUTE "${in}")
endmacro(MAKE_INSTALL_PATH_ABSOLUTE out in)


############### Prepare AkonadiConfig.cmake ###############

# all the following variables are put into AkonadiConfig.cmake, so
# they are usable by projects using Akonadi.
make_install_path_absolute(AKONADI_BIN_DIR             ${BIN_INSTALL_DIR})
make_install_path_absolute(AKONADI_INCLUDE_DIR         ${INCLUDE_INSTALL_DIR})
make_install_path_absolute(AKONADI_LIB_DIR             ${LIB_INSTALL_DIR})
make_install_path_absolute(AKONADI_CONFIG_DIR          ${CONFIG_INSTALL_DIR})
make_install_path_absolute(AKONADI_DBUS_INTERFACES_DIR ${DBUS_INTERFACES_INSTALL_DIR})
make_install_path_absolute(AKONADI_DBUS_SERVICES_DIR   ${DBUS_SERVICES_INSTALL_DIR})
make_install_path_absolute(AKONADI_XDG_MIME_DIR        ${XDG_MIME_INSTALL_DIR})

# Used in configure_file() and install(EXPORT)
#set(AKONADI_TARGET_PREFIX Akonadi__)


############### Generate files ###############

configure_file(akonadi-prefix.h.cmake ${Akonadi_BINARY_DIR}/akonadi-prefix.h)
configure_file(config-akonadi.h.cmake ${Akonadi_BINARY_DIR}/config-akonadi.h)
configure_file(AkonadiConfig.cmake.in "${Akonadi_BINARY_DIR}/AkonadiConfig.cmake" @ONLY)

if (NOT WIN32)
  configure_file(${Akonadi_SOURCE_DIR}/akonadi.pc.cmake ${Akonadi_BINARY_DIR}/akonadi.pc @ONLY)
endif (NOT WIN32)

# this file is used by to check if the installed version can be used.
macro_write_basic_cmake_version_file(${Akonadi_BINARY_DIR}/AkonadiConfigVersion.cmake
                                     ${AKONADI_VERSION_MAJOR} ${AKONADI_VERSION_MINOR} ${AKONADI_VERSION_PATCH})

############### build targets ###############

set( AKONADI_SHARED_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/shared/akapplication.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/shared/akcrash.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/shared/akdebug.cpp
)

include_directories(${Akonadi_SOURCE_DIR} ${Akonadi_BINARY_DIR} ${QT_INCLUDES} ${Boost_INCLUDE_DIR})

add_subdirectory(interfaces)
add_subdirectory(libs)
set( AKONADI_PROTOCOLINTERNALS_LIBS ${akonadiprotocolinternals_LIB_DEPENDS} akonadiprotocolinternals)

if (XSLTPROC_EXECUTABLE)
   add_subdirectory(server)
endif (XSLTPROC_EXECUTABLE)

add_subdirectory(rds)
if (SQLITE_FOUND)
  add_subdirectory(qsqlite)
endif (SQLITE_FOUND)
if (AKONADI_AGENT_SERVER)
  add_subdirectory(agentserver)
endif (AKONADI_AGENT_SERVER)

############### install stuff ###############

if(USE_COMMON_CMAKE_PACKAGE_CONFIG_DIR)
   set(_AkonadiConfig_INSTALL_DIR ${LIB_INSTALL_DIR}/cmake/Akonadi)
else(USE_COMMON_CMAKE_PACKAGE_CONFIG_DIR)
   set(_AkonadiConfig_INSTALL_DIR ${LIB_INSTALL_DIR}/Akonadi/cmake)
endif(USE_COMMON_CMAKE_PACKAGE_CONFIG_DIR)

install(FILES ${Akonadi_BINARY_DIR}/AkonadiConfigVersion.cmake
              ${Akonadi_BINARY_DIR}/AkonadiConfig.cmake
        DESTINATION ${_AkonadiConfig_INSTALL_DIR} )

install(FILES akonadi-mime.xml DESTINATION ${XDG_MIME_INSTALL_DIR})
update_xdg_mimetypes(${XDG_MIME_INSTALL_DIR})

if(NOT WIN32)
  install(FILES ${Akonadi_BINARY_DIR}/akonadi.pc DESTINATION ${LIB_INSTALL_DIR}/pkgconfig)
endif(NOT WIN32)

macro_display_feature_log()
