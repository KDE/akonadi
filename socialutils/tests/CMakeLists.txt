include_directories(${CMAKE_BINARY_DIR}/akonadi/socialutils)

kde4_add_unit_test(asu_tests
  TESTNAME image-provider-tests
  image-provider-tests.cpp
)

target_link_libraries(asu_tests
    akonadi-socialutils
    ${KDE4_KDECORE_LIBS}
    ${QT_QTTEST_LIBRARY}
    ${KDE4_KDEUI_LIBS}
)
