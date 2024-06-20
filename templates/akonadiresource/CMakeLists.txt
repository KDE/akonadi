cmake_minimum_required(VERSION 3.16)

project(%{APPNAMELC})

set(KF_MIN_VERSION "6.0.0")

set(ECM_MIN_VERSION ${KF_MIN_VERSION})
find_package(ECM ${ECM_MIN_VERSION} CONFIG REQUIRED)

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules ${ECM_MODULE_PATH} ${CMAKE_MODULE_PATH})

include(FeatureSummary)
include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDEFrameworkCompilerSettings NO_POLICY_SCOPE)
include(ECMQtDeclareLoggingCategory)

set(QT_MIN_VERSION "6.5.0")
find_package(Qt6 ${QT_MIN_VERSION} REQUIRED Core DBus Gui)

find_package(KF6Config ${KF_MIN_VERSION} CONFIG REQUIRED)

set(AKONADI_MIN_VERSION "6.0.0")
find_package(KF6Akonadi ${AKONADI_MIN_VERSION} CONFIG REQUIRED)

find_program(XSLTPROC_EXECUTABLE xsltproc DOC "Path to the xsltproc executable")
if(NOT XSLTPROC_EXECUTABLE)
    message(FATAL_ERROR "\nThe command line XSLT processor program 'xsltproc'  could not be found.\nPlease install xsltproc.\n")
endif()

add_subdirectory(src)

feature_summary(WHAT ALL INCLUDE_QUIET_PACKAGES FATAL_ON_MISSING_REQUIRED_PACKAGES)
