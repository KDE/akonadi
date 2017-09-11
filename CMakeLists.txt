cmake_minimum_required(VERSION 3.0)

set(PIM_VERSION "5.6.42")
project(Akonadi VERSION ${PIM_VERSION})


# ECM setup
set(KF5_VERSION "5.38.0")

find_package(ECM ${KF5_VERSION} CONFIG REQUIRED)

set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules)

include(GenerateExportHeader)
include(ECMGenerateHeaders)
include(ECMGeneratePriFile)
include(CMakePackageConfigHelpers)
include(ECMSetupVersion)
include(FeatureSummary)
include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDEFrameworkCompilerSettings NO_POLICY_SCOPE)
include(CheckIncludeFiles)
include(ECMQtDeclareLoggingCategory)
include(CheckSymbolExists)
include(ECMCoverageOption)


include(AkonadiMacros)

set(QT_REQUIRED_VERSION "5.8.0")
set(AKONADI_VERSION ${PIM_VERSION})

ecm_setup_version(PROJECT
    VARIABLE_PREFIX AKONADI
    VERSION_HEADER "${CMAKE_CURRENT_BINARY_DIR}/akonadi_version.h"
    PACKAGE_VERSION_FILE "${CMAKE_CURRENT_BINARY_DIR}/KF5AkonadiConfigVersion.cmake"
    SOVERSION 5)

# Find packages
find_package(Qt5Core ${QT_REQUIRED_VERSION} REQUIRED COMPONENTS Private)
find_package(Qt5Sql ${QT_REQUIRED_VERSION} REQUIRED COMPONENTS Private)
find_package(Qt5DBus ${QT_REQUIRED_VERSION} REQUIRED)
find_package(Qt5Network ${QT_REQUIRED_VERSION} REQUIRED)
find_package(Qt5Test ${QT_REQUIRED_VERSION} REQUIRED)
find_package(Qt5Widgets ${QT_REQUIRED_VERSION} REQUIRED)
find_package(Qt5Xml ${QT_REQUIRED_VERSION} REQUIRED)

find_package(KF5Completion ${KF5_VERSION} CONFIG REQUIRED)
find_package(KF5Config ${KF5_VERSION} CONFIG REQUIRED)
find_package(KF5ConfigWidgets ${KF5_VERSION} CONFIG REQUIRED)
find_package(KF5CoreAddons ${KF5_VERSION} CONFIG REQUIRED)
find_package(KF5DBusAddons ${KF5_VERSION} CONFIG REQUIRED)
find_package(KF5I18n ${KF5_VERSION} CONFIG REQUIRED)
find_package(KF5IconThemes ${KF5_VERSION} CONFIG REQUIRED)
find_package(KF5ItemModels ${KF5_VERSION} CONFIG REQUIRED)
find_package(KF5ItemViews ${KF5_VERSION} CONFIG REQUIRED)
find_package(KF5KIO ${KF5_VERSION} CONFIG REQUIRED)
find_package(KF5WidgetsAddons ${KF5_VERSION} CONFIG REQUIRED)
find_package(KF5WindowSystem ${KF5_VERSION} CONFIG REQUIRED)
find_package(KF5XmlGui ${KF5_VERSION} CONFIG REQUIRED)
find_package(KF5Crash ${KF5_VERSION} CONFIG REQUIRED)
if (UNIX AND NOT APPLE)
    find_package(KF5NetworkManagerQt ${KF5_VERSION} CONFIG REQUIRED)
    set(HAVE_NMQT TRUE)
endif()

find_package(Qt5Designer NO_MODULE)
set_package_properties(Qt5Designer PROPERTIES
   PURPOSE "Required to build the Qt Designer plugins"
   TYPE OPTIONAL
)

find_package(KF5DesignerPlugin ${KF5_VERSION} CONFIG)
set_package_properties(KF5DesignerPlugin PROPERTIES DESCRIPTION "KF5 designer plugin" TYPE OPTIONAL)

set(Boost_MINIMUM_VERSION "1.34.0")
find_package(Boost ${Boost_MINIMUM_VERSION})
set_package_properties(Boost PROPERTIES
    DESCRIPTION "Boost C++ Libraries"
    URL "http://www.boost.org"
    TYPE REQUIRED
)


if(BUILD_TESTING)
    set(AKONADI_TESTS_EXPORT AKONADICORE_EXPORT)
endif()
configure_file(akonaditests_export.h.in "${CMAKE_CURRENT_BINARY_DIR}/akonaditests_export.h")

# Make sure the KF5Akonadi_DATA_DIR is absolute before passing it to KF5AkonadiConfig.cmake.in
# otherwise build fails either on OSX CI, or for normal users
if (IS_ABSOLUTE "${KDE_INSTALL_DATADIR_KF5}")
    set(KF5Akonadi_DATA_DIR "${KDE_INSTALL_DATADIR_KF5}/akonadi")
else()
    set(KF5Akonadi_DATA_DIR "${CMAKE_INSTALL_PREFIX}/${KDE_INSTALL_DATADIR_KF5}/akonadi")
endif()

check_symbol_exists(malloc_trim "malloc.h" HAVE_MALLOC_TRIM)

############### Build Options ###############
option(AKONADI_BUILD_QSQLITE "Build the Sqlite backend." TRUE)
option(BUILD_TOOLS "Build and install tools for development and testing purposes." TRUE)

if(BUILD_TESTING)
   set(BUILD_TOOLS TRUE)
endif()

set(SMI_MIN_VERSION "1.0")
find_package(SharedMimeInfo ${SMI_MIN_VERSION} REQUIRED)

find_program(XSLTPROC_EXECUTABLE xsltproc)
if(NOT XSLTPROC_EXECUTABLE)
    message(FATAL_ERROR "\nThe command line XSLT processor program 'xsltproc'  could not be found.\nPlease install xsltproc.\n")
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

find_program(XMLLINT_EXECUTABLE xmllint)
if(NOT XMLLINT_EXECUTABLE)
    message(STATUS "xmllint not found, skipping akonadidb.xml schema validation")
endif()

check_include_files(unistd.h HAVE_UNISTD_H)
if(HAVE_UNISTD_H)
    add_definitions(-DHAVE_UNISTD_H)
endif()

if (IS_ABSOLUTE "${DBUS_INTERFACES_INSTALL_DIR}")
    set(AKONADI_DBUS_INTERFACES_INSTALL_DIR "${DBUS_INTERFACES_INSTALL_DIR}")
else()
    set(AKONADI_DBUS_INTERFACES_INSTALL_DIR "${CMAKE_INSTALL_PREFIX}/${DBUS_INTERFACES_INSTALL_DIR}")
endif()

if (IS_ABSOLUTE "${KDE_INSTALL_INCLUDEDIR_KF5}")
    set(AKONADI_INCLUDE_DIR "${KDE_INSTALL_INCLUDEDIR_KF5}")
else()
    set(AKONADI_INCLUDE_DIR "${CMAKE_INSTALL_PREFIX}/${KDE_INSTALL_INCLUDEDIR_KF5}")
endif()

############### Build Options ###############

include(CTest)  # Calls enable_testing().
include(CTestConfig.cmake)

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

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_C_COMPILER MATCHES "icc" OR (CMAKE_CXX_COMPILER_ID MATCHES "Clang"))
    set(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS} -Wno-long-long -std=iso9899:1990 -Wundef -Wcast-align -Werror-implicit-function-declaration -Wchar-subscripts -Wall -Wextra -Wpointer-arith -Wwrite-strings -Wformat-security -Wmissing-format-attribute -fno-common")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wnon-virtual-dtor -Wundef -Wcast-align -Wchar-subscripts -Wall -Wextra -Wpointer-arith -Wformat-security -fno-common -pedantic")

    set(CMAKE_CXX_FLAGS_DEBUGFULL "-g3 -fno-inline" CACHE STRING "Flags used by the C++ compiler during debugfull builds." FORCE)
    set(CMAKE_C_FLAGS_DEBUGFULL   "-g3 -fno-inline" CACHE STRING "Flags used by the C compiler during debugfull builds." FORCE)
    mark_as_advanced(CMAKE_CXX_FLAGS_DEBUGFULL CMAKE_C_FLAGS_DEBUGFULL)
endif()


if(MSVC)
    set(_ENABLE_EXCEPTIONS -EHsc)
else()
    set(_ENABLE_EXCEPTIONS -fexceptions)
endif()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${_ENABLE_EXCEPTIONS}")

############### Configure files #############

configure_file(akonadi-prefix.h.cmake ${Akonadi_BINARY_DIR}/akonadi-prefix.h)
configure_file(config-akonadi.h.cmake ${Akonadi_BINARY_DIR}/config-akonadi.h)

############### build targets ###############

add_definitions(-DTRANSLATION_DOMAIN=\"libakonadi5\")

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
    src
)

#add_definitions(-DQT_DISABLE_DEPRECATED_BEFORE=0x060000)

add_subdirectory(src)
add_subdirectory(icons)

if(BUILD_TOOLS)
    # add testrunner (application for managing a self-contained
    # test environment)
    add_subdirectory(autotests/libs/testrunner)
    add_subdirectory(autotests/libs/testresource)
    add_subdirectory(autotests/libs/testsearchplugin)
endif()

if(BUILD_TESTING)
    add_subdirectory(autotests)
    add_subdirectory(tests)
endif()


############### install stuff ###############

install(FILES akonadi-mime.xml DESTINATION ${XDG_MIME_INSTALL_DIR})
update_xdg_mimetypes(${XDG_MIME_INSTALL_DIR})

############### CMake Config Files ###############

set(CMAKECONFIG_INSTALL_DIR "${KDE_INSTALL_CMAKEPACKAGEDIR}/KF5Akonadi")

configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/KF5AkonadiConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/KF5AkonadiConfig.cmake"
    INSTALL_DESTINATION ${CMAKECONFIG_INSTALL_DIR}
)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/KF5AkonadiConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/KF5AkonadiConfigVersion.cmake"
    "${CMAKE_CURRENT_SOURCE_DIR}/KF5AkonadiMacros.cmake"
    DESTINATION "${CMAKECONFIG_INSTALL_DIR}"
    COMPONENT Devel
)

install(EXPORT
    KF5AkonadiTargets
    DESTINATION "${CMAKECONFIG_INSTALL_DIR}"
    FILE KF5AkonadiTargets.cmake
    NAMESPACE KF5::)

install(FILES akonadi.categories akonadi.renamecategories
    DESTINATION ${KDE_INSTALL_CONFDIR}
)

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/akonadi_version.h
    DESTINATION ${KDE_INSTALL_INCLUDEDIR_KF5} COMPONENT Devel
)

feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)
