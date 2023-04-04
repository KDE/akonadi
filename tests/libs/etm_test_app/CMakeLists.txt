kde_enable_exceptions()


set(etmtestapp_SRCS
  main.cpp
  mainwindow.cpp mainwindow.h
)

add_executable(akonadi_etm_test_app ${etmtestapp_SRCS})

target_link_libraries(akonadi_etm_test_app
  KPim6::AkonadiWidgets
  KF6::I18n
  akonaditestfake
  Qt::Test
)
