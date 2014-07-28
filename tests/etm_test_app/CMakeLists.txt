include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${Boost_INCLUDE_DIR}
)

kde_enable_exceptions()


set(etmtestapp_SRCS
  main.cpp
  mainwindow.cpp
)

add_executable(akonadi_etm_test_app ${etmtestapp_SRCS})

target_link_libraries(akonadi_etm_test_app
  KF5::AkonadiWidgets
  akonaditestfake
  Qt5::Test
)
