cmake_minimum_required(VERSION 2.8.12)

project(Akonadi)

# ECM setup
find_package(ECM 0.0.11 CONFIG REQUIRED)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH})

include(ECMGenerateHeaders)
include(ECMGeneratePriFile)
include(ECMPackageConfigHelpers)
include(ECMSetupVersion)
include(FeatureSummary)
include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDEFrameworkCompilerSettings)

# FIXME: Do we want the same versions in kdepimlibs and kf5?
set(KF5_VERSION "4.97.0")

ecm_setup_version(${KF5_VERSION}
    VARIABLE_PREFIX AKONADI
    VERSION_HEADER "${CMAKE_CURRENT_BINARY_DIR}/akonadi_version.h"
    PACKAGE_VERSION_FILE "${CMAKE_CURRENT_BINARY_DIR}/KF5AkonadiConfigVersion.cmake"
    SOVERSION 5
)

########### Find packages ###########
find_package(KF5 NO_MODULE REQUIRED COMPONENTS ItemViews KIO Config Solid KDE4Support)

# Workaround for Akonadi server conflicting with our framework (same variable names)
# TODO: Remove once we merge both repositories into one
set(AKONADI_DBUS_INTERFACES_DIR "/usr/share/dbus-1/interfaces")

set(Boost_MINIMUM_VERSION "1.34.0")
find_package(Boost ${Boost_MINIMUM_VERSION})
set_package_properties(Boost PROPERTIES
    DESCRIPTION "Boost C++ Libraries"
    URL "http://www.boost.org"
    TYPE REQUIRED
)

########### Targets ###########

configure_file(akonadiprivate_export.h.in "${CMAKE_CURRENT_BINARY_DIR}/akonadiprivate_export.h")
include_directories(${CMAKE_CURRENT_BINARY_DIR})

add_subdirectory(src)
#add_subdirectory(autotests)


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
    DESTINATION ${INCLUDE_INSTALL_DIR} COMPONENT Devel
)

if ("${CMAKE_BINARY_DIR}" STREQUAL "${CMAKE_CURRENT_BINARY_DIR}")
    feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)
endif()


install( FILES
    kcfg2dbus.xsl
    DESTINATION ${DATA_INSTALL_DIR}/akonadi
)

# if(QT_QTDESIGNER_FOUND)
#   message(STATUS "Building Akonadi widgets")
#   set(akonadiwidgets_PART_SRCS)
#   kde4_add_widget_files(akonadiwidgets_PART_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/designer/akonadi.widgets)
#
#   kde4_add_plugin(akonadiwidgets ${akonadiwidgets_PART_SRCS})
#
#   target_link_libraries(akonadiwidgets ${KDE4_KIO_LIBS} akonadi-kde)
#
#   install(TARGETS akonadiwidgets DESTINATION ${PLUGIN_INSTALL_DIR}/plugins/designer)
# endif()
#
