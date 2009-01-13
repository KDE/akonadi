project (benchmarker)

include_directories(
#    ${CMAKE_SOURCE_DIR}/kabc
#    ${CMAKE_SOURCE_DIR}/kcal
    ${CMAKE_SOURCE_DIR}/akonadi
#    ${CMAKE_BINARY_DIR}/kabc
#    ${CMAKE_BINARY_DIR}/kcal
    ${CMAKE_BINARY_DIR}/akonadi
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${Boost_INCLUDE_DIR}
    ${AKONADI_INCLUDE_DIR}
    ${KDE4_INCLUDES}
)

set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${KDE4_ENABLE_EXCEPTIONS}" )

set(benchmarker_SRCS
  benchmarker.cpp
  test.cpp
  testmaildir.cpp
)

kde4_add_executable(benchmarker ${benchmarker_SRCS})

target_link_libraries(benchmarker
  akonadi-kde
  akonadi-kmime
#  kabc
#  kcal
#  syndication
  ${KDE4_KDEUI_LIBS}
  ${Boost_LIBRARIES}
  ${KDE4_AKONADI_LIBS}
  ${KDE4_KMIME_LIBRARY}
#  ${KDE4_KABC_LIBS}
#  ${KDE4_KCAL_LIBS}
#  ${KDE4_SYNDICATION_LIBS}
  ${KDE4_KDECORE_LIBS}
  ${QT_QTCORE_LIBRARY}
#  ${QT_QTXML_LIBRARY}
  ${QT_QTDBUS_LIBRARY}
  ${QT_QTTEST_LIBRARY}
)

install(TARGETS benchmarker ${INSTALL_TARGETS_DEFAULT_ARGS})
