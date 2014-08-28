project(Akonadi)

cmake_minimum_required(VERSION 2.8.12)

# ECM setup
find_package(ECM 1.1.0 CONFIG REQUIRED)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules)

include(GenerateExportHeader)
include(ECMGeneratePriFile)
include(ECMPackageConfigHelpers)
include(ECMSetupVersion)
include(FeatureSummary)
include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDEFrameworkCompilerSettings)
include(CheckIncludeFiles)

include(AkonadiMacros)

set(KF5_VERSION "5.1.0")
set(AKONADI_VERSION "5.0.40")

ecm_setup_version(${AKONADI_VERSION}
    VARIABLE_PREFIX AKONADI
    VERSION_HEADER "${CMAKE_CURRENT_BINARY_DIR}/akonadi_version.h"
    PACKAGE_VERSION_FILE "${CMAKE_CURRENT_BINARY_DIR}/KF5AkonadiConfigVersion.cmake"
    SOVERSION 5)


# Find packages
find_package(Qt5Core REQUIRED)
find_package(Qt5Gui REQUIRED)
find_package(Qt5Widgets REQUIRED)
find_package(Qt5Sql REQUIRED)
find_package(Qt5Network REQUIRED)
find_package(Qt5Xml REQUIRED)
find_package(Qt5DBus REQUIRED)
find_package(Qt5Test REQUIRED)
# FIXME: Remove DQT5_BUILD from codebase
add_definitions(-DQT5_BUILD)

set(SMI_MIN_VERSION "0.20")
find_package(SharedMimeInfo ${SMI_MIN_VERSION})
set_package_properties(SharedMimeInfo PROPERTIES
    URL "http://freedesktop.org/wiki/Software/shared-mime-info"
    DESCRIPTION "File types database and utilities"
    TYPE REQUIRED
)

find_program(XSLTPROC_EXECUTABLE xsltproc)
if(NOT XSLTPROC_EXECUTABLE)
    message(FATAL_ERROR "\nThe command line XSLT processor program 'xsltproc'  could not be found.\nPlease install xsltproc.\n")
endif()

set(Boost_MINIMUM_VERSION "1.34.0")
find_package(Boost ${Boost_MINIMUM_VERSION} COMPONENTS program_options)
set_package_properties(Boost PROPERTIES
    DESCRIPTION "Boost C++ Libraries"
    URL "http://www.boost.org"
    TYPE REQUIRED
)

# should be handled by FindBoost.cmake ->  cmake bug #8335
# FIXME: Still an issue?
if(WIN32 AND NOT Boost_USE_STATIC_LIBS)
    add_definitions(-DBOOST_DYN_LINK)
    add_definitions(-DBOOST_PROGRAM_OPTIONS_DYN_LINK)
endif()

find_program(MYSQLD_EXECUTABLE NAMES mysqld
    PATHS /usr/sbin /usr/local/sbin /usr/libexec /usr/local/libexec /opt/mysql/libexec /usr/mysql/bin /opt/mysql/sbin
    DOC "The mysqld executable path. ONLY needed at runtime"
)

if(MYSQLD_EXECUTABLE)
    message(STATUS "MySQL Server found: ${MYSQLD_EXECUTABLE}")
else()
    message(STATUS "MySQL Server wasn't found. it is required to use the MySQL backend.")
endif()

find_path(POSTGRES_PATH NAMES pg_ctl
    HINTS /usr/lib${LIB_SUFFIX}/postgresql/8.4/bin
          /usr/lib${LIB_SUFFIX}/postgresql/9.0/bin
          /usr/lib${LIB_SUFFIX}/postgresql/9.1/bin
          DOC "The pg_ctl executable path. ONLY needed at runtime by the PostgreSQL backend"
)

if(POSTGRES_PATH)
    message(STATUS "PostgreSQL Server found.")
else()
    message(STATUS "PostgreSQL wasn't found. it is required to use the Postgres backend.")
endif()


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

find_package(Backtrace)
if(Backtrace_FOUND)
    include_directories(${Backtrace_INCLUDE_DIRS})
    add_definitions(-DHAVE_BACKTRACE)
endif()

find_program(XMLLINT_EXECUTABLE xmllint)
if(NOT XMLLINT_EXECUTABLE)
    message(STATUS "xmllint not found, skipping akonadidb.xml schema validation")
endif()

check_include_files(unistd.h HAVE_UNISTD_H)
if(HAVE_UNISTD_H)
    add_definitions(-DHAVE_UNISTD_H)
endif()

############### Build Options ###############

include(CTest)  # Calls enable_testing().
include(CTestConfig.cmake)
option(AKONADI_BUILD_QSQLITE "Build the Sqlite backend." TRUE)
option(INSTALL_QSQLITE_IN_QT_PREFIX "Install the QSQLite plugin in QT_PLUGIN_DIR" FALSE)

if(NOT DEFINED DATABASE_BACKEND)
    set(DATABASE_BACKEND "MYSQL" CACHE STRING "The default database backend to use for Akonadi. Can be either MYSQL, POSTGRES or SQLITE")
endif()

############### CTest options ###############
# Set a timeout value of 1 minute per test
set(DART_TESTING_TIMEOUT 60)

# CTestCustom.cmake has to be in the CTEST_BINARY_DIR.
# in the KDE build system, this is the same as CMAKE_BINARY_DIR.
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/CTestCustom.cmake ${CMAKE_CURRENT_BINARY_DIR}/CTestCustom.cmake COPYONLY)

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
    add_definitions(-DAKONADI_DATABASE_BACKEND="${AKONADI_DATABASE_BACKEND}")
endmacro()

#### DB BACKEND DEFAULT ####
set_default_db_backend(${DATABASE_BACKEND})

############### Compilers flags ###############

option(CMAKE_COMPILE_GCOV "Build with coverage support." FALSE)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_C_COMPILER MATCHES "icc" OR (CMAKE_CXX_COMPILER_ID MATCHES "Clang"))
    set(_ENABLE_EXCEPTIONS -fexceptions)

    # more aggressive warnings and C++11 support
    set(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS} -Wno-long-long -std=iso9899:1990 -Wundef -Wcast-align -Werror-implicit-function-declaration -Wchar-subscripts -Wall -Wextra -Wpointer-arith -Wwrite-strings -Wformat-security -Wmissing-format-attribute -fno-common")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x -Wnon-virtual-dtor -Wundef -Wcast-align -Wchar-subscripts -Wall -Wextra -Wpointer-arith -Wformat-security -fno-common")

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

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${_ENABLE_EXCEPTIONS}")

add_definitions(-DQT_NO_CAST_FROM_ASCII -DQT_NO_CAST_TO_ASCII)
add_definitions(-DQT_NO_KEYWORDS)
add_definitions(-DQT_USE_FAST_CONCATENATION -DQT_USE_FAST_OPERATOR_PLUS)

############### Configure files #############

configure_file(akonadi-prefix.h.cmake ${Akonadi_BINARY_DIR}/akonadi-prefix.h)

############### build targets ###############

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
    ${Boost_INCLUDE_DIR}
    src
)

add_subdirectory(src)
add_subdirectory(autotests)


############### install stuff ###############

install(FILES akonadi-mime.xml DESTINATION ${XDG_MIME_INSTALL_DIR})
update_xdg_mimetypes(${XDG_MIME_INSTALL_DIR})

feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)

############### CMake Config Files ###############

set(CMAKECONFIG_INSTALL_DIR "${CMAKECONFIG_INSTALL_PREFIX}/KF5Akonadi")

install(EXPORT
    KF5AkonadiTargets
    DESTINATION "${CMAKECONFIG_INSTALL_DIR}"
    FILE KF5AkonadiTargets.cmake
    NAMESPACE KF5::)

