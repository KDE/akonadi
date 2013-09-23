project(etm_test_app)

include_directories(
    ${CMAKE_SOURCE_DIR}/akonadi
    ${CMAKE_BINARY_DIR}/akonadi
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${Boost_INCLUDE_DIR}
    ${AKONADI_INCLUDE_DIR}
    ${KDE4_INCLUDES}
)

set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${KDE4_ENABLE_EXCEPTIONS}" )


set(etmtestapp_SRCS
  main.cpp
  mainwindow.cpp
)

kde4_add_executable(akonadi_etm_test_app ${etmtestapp_SRCS})

target_link_libraries(akonadi_etm_test_app
  akonadi-kde
  akonadi-kmime
  akonaditestfake
  ${AKONADI_COMMON_LIBRARIES}
  ${KDE4_KDEUI_LIBS}
  ${KDE4_AKONADI_LIBS}
  ${KDE4_KMIME_LIBRARY}
  ${KDE4_KDECORE_LIBS}
  ${QT_QTCORE_LIBRARY}
  ${QT_QTDBUS_LIBRARY}
  ${QT_QTNETWORK_LIBRARY}
  ${QT_QTTEST_LIBRARY}
)
