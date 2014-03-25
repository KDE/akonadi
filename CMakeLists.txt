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

ecm_setup_version(${KF5_VERSION} VARIABLE_PREFIX AKONADI
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
  INSTALL_DESTINATION  ${CMAKECONFIG_INSTALL_DIR}
)

install(FILES
  "${CMAKE_CURRENT_BINARY_DIR}/KF5AkonadiConfig.cmake"
  "${CMAKE_CURRENT_BINARY_DIR}/KF5AkonadiConfigVersion.cmake"
  DESTINATION "${CMAKECONFIG_INSTALL_DIR}"
  COMPONENT Devel
)

install(EXPORT KF5AkonadiTargets DESTINATION "${CMAKECONFIG_INSTALL_DIR}" FILE KF5AkonadiTargets.cmake NAMESPACE KF5::)

install(FILES
   ${CMAKE_CURRENT_BINARY_DIR}/akonadi_version.h
  DESTINATION ${INCLUDE_INSTALL_DIR} COMPONENT Devel
)

if ("${CMAKE_BINARY_DIR}" STREQUAL "${CMAKE_CURRENT_BINARY_DIR}")
  feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)
endif()




#
# add_definitions( -DKDE_DEFAULT_DEBUG_AREA=5250 )
#
# set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${KDE4_ENABLE_EXCEPTIONS}" )
# if(CMAKE_COMPILE_GCOV)
#   set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fprofile-arcs -ftest-coverage")
# endif()
#
# if (KDE4_BUILD_TESTS)
#    # add_subdirectory( tests )
# endif()
#
# add_definitions( ${AKONADI_DEFINITIONS} )
# add_definitions( -DQT_NO_CAST_FROM_ASCII )
# add_definitions( -DQT_NO_CAST_TO_ASCII )
#
# # Set the resources and agents version
# configure_file(kdepimlibs-version.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/kdepimlibs-version.h)
#
# # Detect the QJson version (0.8.0 is BIC)
# if (QJSON_VERSION VERSION_GREATER 0.7.1)
#   set(USE_QJSON_0_8 TRUE)
# endif()
#
# configure_file(config-akonadi.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-akonadi.h)
#
# include_directories(
#   ${CMAKE_CURRENT_SOURCE_DIR}
#   ${CMAKE_CURRENT_BINARY_DIR}
# )
# add_subdirectory( kabc )
# message(WARNING "Akonadi/KCal needs to be ported to KCalCore")
# #add_subdirectory( kcal )
# add_subdirectory( kmime )
# # TODO: Remove when Soprano is ported to Qt5.
# add_definitions(-DQ_DECL_CONSTRUCTOR_DEPRECATED=Q_DECL_DEPRECATED)
# # add_subdirectory( contact )
# # add_subdirectory( calendar )
# add_subdirectory( notes )
# # add_subdirectory( socialutils )
# add_subdirectory( xml )
#
# include_directories(
#   ${QT_QTDBUS_INCLUDE_DIR}
#   ${Boost_INCLUDE_DIR}
#   ${KDE4_INCLUDE_DIR}
#   ${QJSON_INCLUDE_DIR}
#   ${AKONADI_INCLUDE_DIR}
#   ${CMAKE_CURRENT_BINARY_DIR}/../kmime
#   ${AKONADI_INCLUDE_DIR}/akonadi/private
# )
#
# # libakonadi-kde
#
# if (KDEPIM_MOBILE_UI)
#   set( akonadikde_LIB_SRC ${akonadikde_LIB_SRC}
#        collectiondialog_mobile.cpp
#   )
# else()
#   set( akonadikde_LIB_SRC ${akonadikde_LIB_SRC}
#        collectiondialog_desktop.cpp
#   )
# endif()
#
# # DBus interfaces and adaptors
# set(akonadi_xml ${AKONADI_DBUS_INTERFACES_DIR}/org.freedesktop.Akonadi.NotificationManager.xml)
# set_source_files_properties(${akonadi_xml} PROPERTIES INCLUDE "notificationmessagev3_p.h" )
# qt4_add_dbus_interface( akonadikde_LIB_SRC ${akonadi_xml} notificationmanagerinterface )
#
# set(akonadi_xml ${AKONADI_DBUS_INTERFACES_DIR}/org.freedesktop.Akonadi.NotificationSource.xml)
# set_source_files_properties(${akonadi_xml} PROPERTIES INCLUDE "notificationmessagev3_p.h" )
# qt4_add_dbus_interface( akonadikde_LIB_SRC ${akonadi_xml} notificationsourceinterface )
#
# qt4_add_dbus_interfaces( akonadikde_LIB_SRC ${AKONADI_DBUS_INTERFACES_DIR}/org.freedesktop.Akonadi.AgentManager.xml )
# qt4_add_dbus_interfaces( akonadikde_LIB_SRC ${AKONADI_DBUS_INTERFACES_DIR}/org.freedesktop.Akonadi.Tracer.xml )
# qt4_add_dbus_interfaces( akonadikde_LIB_SRC ${AKONADI_DBUS_INTERFACES_DIR}/org.freedesktop.Akonadi.Agent.Control.xml )
# qt4_add_dbus_adaptor( akonadikde_LIB_SRC ${AKONADI_DBUS_INTERFACES_DIR}/org.freedesktop.Akonadi.Resource.xml resourcebase.h Akonadi::ResourceBase resourceadaptor Akonadi__ResourceAdaptor )
# qt4_add_dbus_adaptor( akonadikde_LIB_SRC ${AKONADI_DBUS_INTERFACES_DIR}/org.freedesktop.Akonadi.Preprocessor.xml preprocessorbase_p.h Akonadi::PreprocessorBasePrivate preprocessoradaptor Akonadi__PreprocessorAdaptor )
# qt4_add_dbus_adaptor( akonadikde_LIB_SRC ${AKONADI_DBUS_INTERFACES_DIR}/org.freedesktop.Akonadi.Agent.Status.xml agentbase.h Akonadi::AgentBase statusadaptor Akonadi__StatusAdaptor )
# qt4_add_dbus_adaptor( akonadikde_LIB_SRC ${AKONADI_DBUS_INTERFACES_DIR}/org.freedesktop.Akonadi.Agent.Control.xml agentbase.h Akonadi::AgentBase controladaptor Akonadi__ControlAdaptor)
# qt4_add_dbus_adaptor( akonadikde_LIB_SRC interfaces/org.freedesktop.Akonadi.Resource.Transport.xml transportresourcebase_p.h Akonadi::TransportResourceBasePrivate transportadaptor Akonadi__TransportAdaptor )
# qt4_add_dbus_adaptor( akonadikde_LIB_SRC interfaces/org.freedesktop.Akonadi.Agent.Search.xml agentsearchinterface_p.h Akonadi::AgentSearchInterfacePrivate searchadaptor Akonadi__SearchAdaptor )
#
#
# kde4_add_kcfg_files( akonadikde_LIB_SRC resourcebasesettings.kcfgc )
# install( FILES resourcebase.kcfg DESTINATION ${KCFG_INSTALL_DIR} )
#
# kde4_add_ui_files( akonadikde_LIB_SRC
#   cachepolicypage.ui
#   controlprogressindicator.ui
#   selftestdialog.ui
#   erroroverlay.ui
# )
#
# if (KDEPIM_MOBILE_UI)
#   kde4_add_ui_files( akonadikde_LIB_SRC
#     collectiongeneralpropertiespage_mobile.ui
#   )
# else()
#   kde4_add_ui_files( akonadikde_LIB_SRC
#     collectiongeneralpropertiespage.ui
#   )
# endif()
#
# add_definitions(
#   -DAKONADI_DEPRECATED=
# )
# # This one won't be needed when CMake 2.8.13 is depended on.
# add_definitions(
#   -DAKONADI_DEPRECATED_EXPORT=AKONADI_EXPORT
# )
#
# add_library( akonadi-kde ${LIBRARY_TYPE} ${akonadikde_LIB_SRC} )
# set_property(TARGET akonadi-kde PROPERTY DEFINE_SYMBOL akonadi_kde)
#
# generate_export_header(akonadi-kde BASE_NAME akonadi)
#
# if (KDE4_BUILD_TESTS)
#    set(AKONADI_TESTS_EXPORT AKONADI_EXPORT)
# endif()
# configure_file(akonadiprivate_export.h.in "${CMAKE_CURRENT_BINARY_DIR}/akonadiprivate_export.h")
#
# if (KDE4_BUILD_TESTS)
#    set(AKONADI_TESTS_EXPORT AKONADI_EXPORT)
# endif()
# configure_file(akonadiprivate_export.h.in
#   "${CMAKE_CURRENT_BINARY_DIR}/akonadiprivate_export.h"
# )
#
# target_link_libraries( akonadi-kde PRIVATE Qt5::Network Qt5::Sql KF5::KIOCore KF5::Solid KF5::KDE4Support )
# set( AKONADI_KDE_DEPS Qt5::Core KF5::ItemViews Qt5::DBus )
#
# if (KDEPIM_MOBILE_UI)
# target_link_libraries( akonadi-kde Qt5::Widgets)
# endif()
#
# target_link_libraries( akonadi-kde ${AKONADI_KDE_DEPS})
#
# set_target_properties( akonadi-kde PROPERTIES VERSION ${GENERIC_LIB_VERSION} SOVERSION ${GENERIC_LIB_SOVERSION} )
# install( TARGETS akonadi-kde EXPORT kdepimlibsLibraryTargets  ${INSTALL_TARGETS_DEFAULT_ARGS} )
#
# ########### install files ###############
#

# install( FILES
#   kcfg2dbus.xsl
#   DESTINATION ${DATA_INSTALL_DIR}/akonadi-kde
# )
#
# if (KDEPIM_MOBILE_UI)
# install( FILES
#   CollectionDialogMobile.qml
#   DESTINATION ${DATA_INSTALL_DIR}/akonadi-kde/qml
# )
# endif()
#
# set(module_install_dir ${DATA_INSTALL_DIR}/cmake/modules )
# install( FILES
#   AkonadiMacros.cmake
#   DESTINATION ${module_install_dir}
# )
#
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
