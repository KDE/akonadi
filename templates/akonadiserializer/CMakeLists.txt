cmake_minimum_required(VERSION 3.1)

project(%{APPNAMELC})

set(ECM_MIN_VERSION "5.22.0")
find_package(ECM ${ECM_MIN_VERSION} CONFIG REQUIRED)

set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH} ${CMAKE_MODULE_PATH})

include(FeatureSummary)
include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDEFrameworkCompilerSettings NO_POLICY_SCOPE)

set(QT_MIN_VERSION "5.6.0")
find_package(Qt5 ${QT_MIN_VERSION} REQUIRED Core Network Gui)

set(KF5_MIN_VERSION "5.22.0")
find_package(KF5Config ${KF5_MIN_VERSION} CONFIG REQUIRED)

set(AKONADI_MIN_VERSION "5.2")
find_package(KF5Akonadi ${AKONADI_MIN_VERSION} CONFIG REQUIRED)

add_subdirectory(src)

feature_summary(WHAT ALL INCLUDE_QUIET_PACKAGES FATAL_ON_MISSING_REQUIRED_PACKAGES)
