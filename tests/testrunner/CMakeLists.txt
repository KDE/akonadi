project (akonaditest)

include_directories(
    ${CMAKE_SOURCE_DIR}/akonadi
    ${CMAKE_BINARY_DIR}/akonadi
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${Boost_INCLUDE_DIR}
    ${AKONADI_INCLUDE_DIR}
    ${KDE4_INCLUDES}
)

set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${KDE4_ENABLE_EXCEPTIONS}" )

set(akonaditest_SRCS
  main.cpp
  setup.cpp
  config.cpp
  shellscript.cpp
  testrunner.cpp
)

kde4_add_executable(akonaditest NOGUI ${akonaditest_SRCS})

target_link_libraries(akonaditest
  akonadi-kde
  ${KDE4_KDEUI_LIBS}
  ${KDE4_AKONADI_LIBS}
  ${QT_QTXML_LIBRARY}
  ${QT_QTDBUS_LIBRARY}
)

install(TARGETS akonaditest ${KF5_INSTALL_TARGETS_DEFAULT_ARGS})

# Set the akonaditest path (needed by AkonadiMacros.cmake when invoked in kdepimlibs)
set(_akonaditest_DIR ${EXECUTABLE_OUTPUT_PATH} CACHE PATH "akonaditest path")

