kde_enable_exceptions()

set(akonaditest_SRCS
  main.cpp
  setup.cpp setup.h
  config.cpp
  shellscript.cpp shellscript.h
  testrunner.cpp testrunner.h
)

ecm_qt_declare_logging_category(akonaditest_SRCS HEADER akonaditest_debug.h IDENTIFIER AKONADITEST_LOG CATEGORY_NAME org.kde.pim.akonaditest)

add_executable(akonaditest ${akonaditest_SRCS})

target_link_libraries(akonaditest
  KPim${KF_MAJOR_VERSION}::AkonadiPrivate
  KPim${KF_MAJOR_VERSION}::AkonadiCore
  KF${KF_MAJOR_VERSION}::I18n
  KF${KF_MAJOR_VERSION}::ConfigCore
  Qt::Xml
  Qt::DBus
  Qt::Widgets
)

install(TARGETS akonaditest ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})

# Set the akonaditest path (needed by AkonadiMacros.cmake when invoked in kdepimlibs)
set(_akonaditest_DIR ${CMAKE_CURRENT_BINARY_DIR} CACHE PATH "akonaditest path")
