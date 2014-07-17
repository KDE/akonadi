project(etm_test_app)

include_directories(
    ${CMAKE_SOURCE_DIR}/akonadi
    ${CMAKE_BINARY_DIR}/akonadi
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${Boost_INCLUDE_DIR}
    ${AKONADI_INCLUDE_DIR}
)

kde_enable_exceptions()


set(etmtestapp_SRCS
  main.cpp
  mainwindow.cpp
)

add_executable(akonadi_etm_test_app ${etmtestapp_SRCS})

target_link_libraries(akonadi_etm_test_app
  KF5::AkonadiMime
  akonaditestfake
  ${AKONADI_COMMON_LIBRARIES}
  Qt5::Test
)
