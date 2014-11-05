cmake_minimum_required(VERSION 2.8.12)

project(Akonadi)

# ECM setup
find_package(ECM 1.3.0 CONFIG REQUIRED)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH})

include(GenerateExportHeader)
include(ECMGenerateHeaders)
include(ECMGeneratePriFile)
include(ECMPackageConfigHelpers)
include(ECMSetupVersion)
include(FeatureSummary)
include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDEFrameworkCompilerSettings)

# FIXME: Do we want the same versions in kdepimlibs and kf5?
set(KF5_VERSION "5.4.0")
set(KDEPIMLIBS_VERSION "4.73.0")
set(QT_REQUIRED_VERSION "5.2.0")

ecm_setup_version(${KDEPIMLIBS_VERSION}
    VARIABLE_PREFIX AKONADILIBRARIES
    VERSION_HEADER "${CMAKE_CURRENT_BINARY_DIR}/akonadi_version.h"
    PACKAGE_VERSION_FILE "${CMAKE_CURRENT_BINARY_DIR}/KF5AkonadiConfigVersion.cmake"
    SOVERSION 5
)

########### Find packages ###########
find_package(KF5ItemViews ${KF5_VERSION} CONFIG REQUIRED)
find_package(KF5KIO ${KF5_VERSION} CONFIG REQUIRED)
find_package(KF5Config ${KF5_VERSION} CONFIG REQUIRED)
find_package(KF5Solid ${KF5_VERSION} CONFIG REQUIRED)
find_package(KF5KDELibs4Support ${KF5_VERSION} CONFIG REQUIRED)


set(AKONADI_TESTS_EXPORT AKONADICORE_EXPORT)
configure_file(akonadiprivate_export.h.in "${CMAKE_CURRENT_BINARY_DIR}/akonadiprivate_export.h")


set(KF5AkonadiServer_MIN_VERSION "5.0.40")
find_package(KF5AkonadiServer ${KF5AkonadiServer_MIN_VERSION} CONFIG REQUIRED)

find_package(Qt5Designer NO_MODULE)
set_package_properties(Qt5Designer PROPERTIES
   PURPOSE "Required to build the Qt Designer plugins"
   TYPE OPTIONAL
)

find_package(Qt5Sql NO_MODULE REQUIRED)

set(Boost_MINIMUM_VERSION "1.34.0")
find_package(Boost ${Boost_MINIMUM_VERSION})
set_package_properties(Boost PROPERTIES
    DESCRIPTION "Boost C++ Libraries"
    URL "http://www.boost.org"
    TYPE REQUIRED
)

find_package(LibXml2)
set_package_properties(LibXml2 PROPERTIES
    DESCRIPTION "The LibXML2 library"
    URL "http://xmlsoft.org"
    TYPE REQUIRED
)

########### Targets ###########
add_definitions(-DTRANSLATION_DOMAIN=\"libakonadi5\")
add_definitions("-DQT_NO_CAST_FROM_ASCII -DQT_NO_CAST_TO_ASCII")

configure_file(akonadiprivate_export.h.in "${CMAKE_CURRENT_BINARY_DIR}/akonadiprivate_export.h")

add_subdirectory(src)
add_subdirectory(autotests)
add_subdirectory(tests)

########### CMake Config Files ###########

set(CMAKECONFIG_INSTALL_DIR "${CMAKECONFIG_INSTALL_PREFIX}/KF5Akonadi")

ecm_configure_package_config_file(
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

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/akonadi_version.h
    DESTINATION ${KF5_INCLUDE_INSTALL_DIR} COMPONENT Devel
)

if ("${CMAKE_BINARY_DIR}" STREQUAL "${CMAKE_CURRENT_BINARY_DIR}")
    feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)
endif()

