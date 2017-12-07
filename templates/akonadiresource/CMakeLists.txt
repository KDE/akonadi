cmake_minimum_required(VERSION 3.1)

project(%{APPNAMELC})

set(KF5_MIN_VERSION "5.22.0")

set(ECM_MIN_VERSION ${KF5_MIN_VERSION})
find_package(ECM ${ECM_MIN_VERSION} CONFIG REQUIRED)

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules ${ECM_MODULE_PATH} ${CMAKE_MODULE_PATH})

include(FeatureSummary)
include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDEFrameworkCompilerSettings NO_POLICY_SCOPE)
include(ECMQtDeclareLoggingCategory)

set(QT_MIN_VERSION "5.7.0")
find_package(Qt5 ${QT_MIN_VERSION} REQUIRED Core DBus Gui)

find_package(KF5Config ${KF5_MIN_VERSION} CONFIG REQUIRED)

set(AKONADI_MIN_VERSION "5.2")
find_package(KF5Akonadi ${AKONADI_MIN_VERSION} CONFIG REQUIRED)

find_package(Xsltproc REQUIRED)
set_package_properties(Xsltproc PROPERTIES
    DESCRIPTION "XSLT processor from libxslt"
    TYPE REQUIRED
    PURPOSE "Required to generate a D-Bus interface for the resource."
)

add_subdirectory(src)

feature_summary(WHAT ALL INCLUDE_QUIET_PACKAGES FATAL_ON_MISSING_REQUIRED_PACKAGES)
