kde_enable_exceptions()


set(etmtestapp_SRCS
  main.cpp
  mainwindow.cpp
)

add_executable(akonadi_etm_test_app ${etmtestapp_SRCS})

target_link_libraries(akonadi_etm_test_app
  KF5::AkonadiWidgets
  KF5::I18n
  akonaditestfake
  Qt::Test
)
