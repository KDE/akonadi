cmake_minimum_required(VERSION 2.8.9)
project(Akonadi)

# where to look first for cmake modules, before ${CMAKE_ROOT}/Modules/ is checked
set(CMAKE_MODULE_PATH "${Akonadi_SOURCE_DIR}/cmake/modules")

# Enable CMake's Automoc
set(CMAKE_AUTOMOC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

# Used to report the packages we found
include(FeatureSummary)

############### Build Options ###############

include(CTest)  # Calls enable_testing().
include(CTestConfig.cmake)
option(AKONADI_BUILD_TESTS "Build the Akonadi unit tests." TRUE)
option(AKONADI_BUILD_QSQLITE "Build the Sqlite backend." TRUE)
option(INSTALL_QSQLITE_IN_QT_PREFIX "Install the QSQLite plugin in QT_PLUGIN_DIR" FALSE)
option(STATIC_LIBRARY "Build Akonadi as a static library." FALSE)
option(QT5_BUILD "Build Akonadi using the Qt5 framework" FALSE)
option(WITH_SOPRANO "Build with Soprano support. Needed for Nepomuk search integration." FALSE)

if(NOT DEFINED DATABASE_BACKEND)
  set(DATABASE_BACKEND "MYSQL" CACHE STRING "The default database backend to use for Akonadi. Can be either MYSQL, POSTGRES or SQLITE")
endif()

############### CMake Macros ###############

include(InstallSettings)
include(CheckFunctionExists)
include(CheckIncludeFiles)
include(CMakePackageConfigHelpers)

############### CTest options ###############
# Set a timeout value of 1 minute per test
set(DART_TESTING_TIMEOUT 60)

# CTestCustom.cmake has to be in the CTEST_BINARY_DIR.
# in the KDE build system, this is the same as CMAKE_BINARY_DIR.
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/CTestCustom.cmake ${CMAKE_CURRENT_BINARY_DIR}/CTestCustom.cmake COPYONLY)

############### The Akonadi version (used in AkonadiConfig.cmake) ###############

set(AKONADI_VERSION_PATCH "0")

# Raise the minor version if we're building Akonadi using Qt5
if(QT5_BUILD)
  set(AKONADI_VERSION_MAJOR "1")
  set(AKONADI_VERSION_MINOR "79")
  set(AKONADI_SOVERSION "2")
else ()
  set(AKONADI_VERSION_MAJOR "1")
  set(AKONADI_VERSION_MINOR "13")
  set(AKONADI_SOVERSION "1")
endif()

set(AKONADI_VERSION "${AKONADI_VERSION_MAJOR}.${AKONADI_VERSION_MINOR}.${AKONADI_VERSION_PATCH}")
set(AKONADI_VERSION_STRING "${AKONADI_VERSION}")

# If Git is installed and a '.git' directory is found,
# we append the Git revision to AKONADI_VERSION_STRING
if(EXISTS "${Akonadi_SOURCE_DIR}/.git")
  find_package(Git)
  if(GIT_FOUND)
    message(STATUS "Found git: ${GIT_EXECUTABLE}")
    execute_process(COMMAND ${GIT_EXECUTABLE} rev-parse --short HEAD
                    WORKING_DIRECTORY ${Akonadi_SOURCE_DIR}
                    OUTPUT_VARIABLE akonadi_git_revision
                    OUTPUT_STRIP_TRAILING_WHITESPACE)
    set(AKONADI_VERSION_STRING "${AKONADI_VERSION_STRING} (revision: ${akonadi_git_revision})")
  endif()
endif()


############### Macros ###############

macro(SET_DEFAULT_DB_BACKEND)
    set(_backend ${ARGV0})
    if("${_backend}" STREQUAL "SQLITE")
        set(AKONADI_DATABASE_BACKEND QSQLITE3)
        set(AKONADI_BUILD_QSQLITE TRUE)
    else()
        if("${_backend}" STREQUAL "POSTGRES")
          set(AKONADI_DATABASE_BACKEND QPSQL)
        else()
          set(AKONADI_DATABASE_BACKEND QMYSQL)
        endif()
    endif()

    message(STATUS "Using default db backend ${AKONADI_DATABASE_BACKEND}")
endmacro()

#### DB BACKEND DEFAULT ####
set_default_db_backend(${DATABASE_BACKEND})


############### Find what we need ###############

#### Qt 4 and 5 ####
if(QT5_BUILD)
  find_package(Qt5Core REQUIRED)
  find_package(Qt5Gui REQUIRED)
  find_package(Qt5Widgets REQUIRED)
  find_package(Qt5Sql REQUIRED)
  find_package(Qt5Network REQUIRED)
  find_package(Qt5Xml REQUIRED)
  find_package(Qt5DBus REQUIRED)
  find_package(Qt5Test REQUIRED)

  include("cmake/modules/ECMQt4To5Porting.cmake")
  include_directories(${QT_INCLUDES}) # TODO: Port away from this.

  if(CMAKE_VERSION VERSION_LESS 2.8.9)
    message(FATAL_ERROR "Akonadi Qt 5 build requires at least CMake version 2.8.9")
  endif()

  if (Qt5_POSITION_INDEPENDENT_CODE)
    set(CMAKE_POSITION_INDEPENDENT_CODE ON)
  endif()

  set(QT_QTTEST_LIBRARIES Qt5::Test)
else()
  set(QT_USE_IMPORTED_TARGETS TRUE) # Qt 4 only
  set(QT_MIN_VERSION 4.8.0)         # Qt 4 only

  find_package(Qt4 REQUIRED)
  include(${QT_USE_FILE})
endif()

if(STATIC_LIBRARY)
  set(LIBRARY_TYPE STATIC)
  set(AKONADI_STATIC_LIBS ON)
  message(STATUS "Building Akonadi as a static library")
else()
  set(LIBRARY_TYPE SHARED)
endif()

#### Soprano ####
if(WITH_SOPRANO)
  set(SOPRANO_MIN_VERSION 2.7.56)
  find_package(Soprano)

  set_package_properties(Soprano PROPERTIES
                         URL "http://soprano.sourceforge.net"
                         DESCRIPTION "Storage of semantic data"
                         TYPE REQUIRED
                         PURPOSE "Soprano is needed for the Nepomuk search backend"
  )
endif()

#### SMI ####
set(SMI_MIN_VERSION "0.20")
find_package(SharedMimeInfo ${SMI_MIN_VERSION})
set_package_properties(SharedMimeInfo PROPERTIES
                       URL "http://freedesktop.org/wiki/Software/shared-mime-info"
                       DESCRIPTION "File types database and utilities"
                       TYPE REQUIRED
)

#### XSLTProc ####
find_program(XSLTPROC_EXECUTABLE xsltproc)
if(NOT XSLTPROC_EXECUTABLE)
  message(FATAL_ERROR "\nThe command line XSLT processor program 'xsltproc'  could not be found.\nPlease install xsltproc.\n")
endif()

#### Boost ####
# In CMake >= 2.8.6, FindBoost.cmake tries to find BoostConfig.cmake which is
# not compatible with CMake's FindBoost. Disable this function.
set(Boost_NO_BOOST_CMAKE TRUE)

find_package(Boost COMPONENTS program_options)
set_package_properties(Boost PROPERTIES
                       URL "http://www.boost.org"
                       DESCRIPTION "The Boost C++ Libraries"
                       TYPE REQUIRED
                       PURPOSE "Akonadi requires the Boost C++ libraries (program_options)"
)

# should be handled by FindBoost.cmake ->  cmake bug #8335
if(WIN32 AND NOT Boost_USE_STATIC_LIBS)
  add_definitions(-DBOOST_DYN_LINK)
  add_definitions(-DBOOST_PROGRAM_OPTIONS_DYN_LINK)
endif()


#### Sqlite ####
# If Sqlite is the default backend, it cannot be optional.
if("${DATABASE_BACKEND}" STREQUAL "SQLITE")
  set(SQLITE_TYPE "REQUIRED")
else()
  set(SQLITE_TYPE "OPTIONAL")
endif()

if(AKONADI_BUILD_QSQLITE)
  set(SQLITE_MIN_VERSION 3.6.23)
  find_package(Sqlite ${SQLITE_MIN_VERSION})
  set_package_properties(Sqlite PROPERTIES
                         URL "http://www.sqlite.org"
                         DESCRIPTION "The Sqlite database library"
                         TYPE ${SQLITE_TYPE}
                         PURPOSE "Required by the Sqlite backend"
  )
endif()

############### Compilers flags ###############

option(CMAKE_COMPILE_GCOV "Build with coverage support." FALSE)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_C_COMPILER MATCHES "icc" OR (CMAKE_CXX_COMPILER_ID MATCHES "Clang"))
  set(_ENABLE_EXCEPTIONS -fexceptions)

  # more aggressive warnings and C++11 support
  set(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS} -Wno-long-long -std=iso9899:1990 -Wundef -Wcast-align -Werror-implicit-function-declaration -Wchar-subscripts -Wall -Wextra -Wpointer-arith -Wwrite-strings -Wformat-security -Wmissing-format-attribute -fno-common")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x -Wnon-virtual-dtor -Wundef -Wcast-align -Wchar-subscripts -Wall -Wextra -Wpointer-arith -Wformat-security -fno-common")

  file(WRITE ${CMAKE_BINARY_DIR}/cxx11_check.cpp
       "enum Enum { Value = 1 };
        struct Class {
            Class(int val) { (void)val; };
            // Delegating constructor
            Class(): Class(42) {};
            // New-style enumerator
            Class(Enum e = Enum::Value) { (void)e; };
        };
        int main() {}
       ")
  try_compile(CXX11_SUPPORTED
              ${CMAKE_BINARY_DIR}/cxx11_check
              ${CMAKE_BINARY_DIR}/cxx11_check.cpp)
  if (NOT CXX11_SUPPORTED)
      message(FATAL_ERROR "Compiler does not support all required C++11 features")
  endif()

  # debugfull target
  set(CMAKE_CXX_FLAGS_DEBUGFULL "-g3 -fno-inline" CACHE STRING "Flags used by the C++ compiler during debugfull builds." FORCE)
  set(CMAKE_C_FLAGS_DEBUGFULL   "-g3 -fno-inline" CACHE STRING "Flags used by the C compiler during debugfull builds." FORCE)
  mark_as_advanced(CMAKE_CXX_FLAGS_DEBUGFULL CMAKE_C_FLAGS_DEBUGFULL)

  # Update the documentation string of CMAKE_BUILD_TYPE for ccache & cmake-gui
  set(CMAKE_BUILD_TYPE "${CMAKE_BUILD_TYPE}" CACHE STRING
      "Choose the type of build, options are: None debugfull Debug Release RelWithDebInfo MinSizeRel."
      FORCE)

  # coverage support
  if(CMAKE_COMPILE_GCOV)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fprofile-arcs -ftest-coverage")
    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
      set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -lprofile_rt" CACHE STRING "Flags used by the linker" FORCE)
      set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} -lprofile_rt" CACHE STRING "Flags used by the linker" FORCE)
      set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -lprofile_rt" CACHE STRING "Flags used by the linker" FORCE)
    endif()
  endif()
endif()

if(MSVC)
  set(_ENABLE_EXCEPTIONS -EHsc)
endif()

add_definitions(-DQT_NO_CAST_FROM_ASCII -DQT_NO_CAST_TO_ASCII)
add_definitions(-DQT_NO_KEYWORDS)
add_definitions(-DQT_USE_FAST_CONCATENATION -DQT_USE_FAST_OPERATOR_PLUS)


############### Configure checks ###############

set(AKONADI_SYSTEM_LIBS)

find_package(Backtrace)
if(Backtrace_FOUND)
  include_directories(${Backtrace_INCLUDE_DIRS})
  set(AKONADI_SYSTEM_LIBS ${AKONADI_SYSTEM_LIBS} ${Backtrace_LIBRARIES})
endif()

check_include_files(unistd.h HAVE_UNISTD_H)

# set the output paths
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin)
if(WIN32)
  set(LIBRARY_OUTPUT_PATH   ${EXECUTABLE_OUTPUT_PATH})
else()
  set(LIBRARY_OUTPUT_PATH   ${CMAKE_BINARY_DIR}/lib)
endif()

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
endif()

if(NOT DEFINED AKONADI_BUNDLE_PATH)
  set(AKONADI_BUNDLE_PATH "/Applications/KDE4")
endif()

if(APPLE)
   message(STATUS "MacOS Bundle Path: ${AKONADI_BUNDLE_PATH}")
   set(CMAKE_INSTALL_NAME_DIR ${LIB_INSTALL_DIR})
endif()

############### Generate files ###############
# Used in configure_file() and install(EXPORT). Must be set before setting the AkonadiConfig.cmake vars.
set(AKONADI_TARGET_PREFIX Akonadi__)

if(Soprano_FOUND)
  set(HAVE_SOPRANO TRUE)
endif()
configure_file(akonadi-prefix.h.cmake ${Akonadi_BINARY_DIR}/akonadi-prefix.h)
configure_file(config-akonadi.h.cmake ${Akonadi_BINARY_DIR}/config-akonadi.h)

# This command will replace the installation dirs with absolute paths in AkonadiConfig.cmake
configure_package_config_file(AkonadiConfig.cmake.in ${Akonadi_BINARY_DIR}/AkonadiConfig.cmake
                              INSTALL_DESTINATION ${LIB_INSTALL_DIR}/cmake/Akonadi
                              PATH_VARS BIN_INSTALL_DIR INCLUDE_INSTALL_DIR
                              LIB_INSTALL_DIR CONFIG_INSTALL_DIR
                              DBUS_INTERFACES_INSTALL_DIR DBUS_SERVICES_INSTALL_DIR
                              SHARE_INSTALL_PREFIX XDG_MIME_INSTALL_DIR
                             )

# this file is used by to check if the installed version can be used.
write_basic_package_version_file(${Akonadi_BINARY_DIR}/AkonadiConfigVersion.cmake
                                 VERSION ${AKONADI_VERSION}
                                 COMPATIBILITY SameMajorVersion
                                )

if(NOT WIN32)
  configure_file(${Akonadi_SOURCE_DIR}/akonadi.pc.cmake ${Akonadi_BINARY_DIR}/akonadi.pc @ONLY)
endif()

############### build targets ###############

include_directories(${Akonadi_SOURCE_DIR} ${Akonadi_BINARY_DIR} ${QT_INCLUDES} ${Boost_INCLUDE_DIR})

add_subdirectory(interfaces)
add_subdirectory(libs)
set(AKONADI_PROTOCOLINTERNALS_LIBS ${akonadiprotocolinternals_LIB_DEPENDS} akonadiprotocolinternals)

add_subdirectory(shared)
add_subdirectory(agentserver)
add_subdirectory(server)

add_subdirectory(rds)
if(NOT WIN32)
  add_subdirectory(asapcat)
endif()
if (NOT QT5_BUILD)
  if(SQLITE_FOUND)
    option(SQLITE_LINK_STATIC "link libsqlite3 statically" FALSE)
    add_subdirectory(qsqlite)
  endif()
endif()

############### install stuff ###############

install(FILES ${Akonadi_BINARY_DIR}/AkonadiConfigVersion.cmake
              ${Akonadi_BINARY_DIR}/AkonadiConfig.cmake
        DESTINATION ${LIB_INSTALL_DIR}/cmake/Akonadi)

install(FILES akonadi-mime.xml DESTINATION ${XDG_MIME_INSTALL_DIR})
update_xdg_mimetypes(${XDG_MIME_INSTALL_DIR})

if(NOT WIN32)
  install(FILES ${Akonadi_BINARY_DIR}/akonadi.pc DESTINATION ${LIB_INSTALL_DIR}/pkgconfig)
endif()


feature_summary(WHAT ALL
                     INCLUDE_QUIET_PACKAGES
                     FATAL_ON_MISSING_REQUIRED_PACKAGES
               )

# Install the file with the exported targets
install(EXPORT akonadiLibraryTargets
        NAMESPACE ${AKONADI_TARGET_PREFIX}
        DESTINATION ${LIB_INSTALL_DIR}/cmake/Akonadi
        FILE AkonadiTargetsWithPrefix.cmake)
