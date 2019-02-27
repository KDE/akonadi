kde_enable_exceptions()

set(akonaditest_SRCS
  main.cpp
  setup.cpp
  config.cpp
  shellscript.cpp
  testrunner.cpp
)

add_executable(akonaditest ${akonaditest_SRCS})

target_link_libraries(akonaditest
  KF5::AkonadiPrivate
  KF5::AkonadiCore 
  KF5::I18n
  KF5::ConfigCore
  Qt5::Xml
  Qt5::DBus
  Qt5::Widgets
)

install(TARGETS akonaditest ${KF5_INSTALL_TARGETS_DEFAULT_ARGS})

# Set the akonaditest path (needed by AkonadiMacros.cmake when invoked in kdepimlibs)
set(_akonaditest_DIR ${CMAKE_CURRENT_BINARY_DIR} CACHE PATH "akonaditest path")
