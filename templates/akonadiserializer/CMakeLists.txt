cmake_minimum_required(VERSION 3.16)

project(%{APPNAMELC})

set(KF5_MIN_VERSION "5.82.0")

set(ECM_MIN_VERSION ${KF5_MIN_VERSION})
find_package(ECM ${ECM_MIN_VERSION} CONFIG REQUIRED)

set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH} ${CMAKE_MODULE_PATH})

include(FeatureSummary)
include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDEFrameworkCompilerSettings NO_POLICY_SCOPE)

set(QT_MIN_VERSION "5.15.0")
find_package(Qt5 ${QT_MIN_VERSION} REQUIRED Core Network Gui)

find_package(KF5Config ${KF5_MIN_VERSION} CONFIG REQUIRED)

set(AKONADI_MIN_VERSION "5.16.0")
find_package(KF5Akonadi ${AKONADI_MIN_VERSION} CONFIG REQUIRED)

add_subdirectory(src)

feature_summary(WHAT ALL INCLUDE_QUIET_PACKAGES FATAL_ON_MISSING_REQUIRED_PACKAGES)
