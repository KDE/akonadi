project (benchmarker)

include_directories(
    ${CMAKE_SOURCE_DIR}/akonadi
    ${CMAKE_BINARY_DIR}/akonadi
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${Boost_INCLUDE_DIR}
    ${AKONADI_INCLUDE_DIR}
    ${KDE4_INCLUDES}
)

set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${KDE4_ENABLE_EXCEPTIONS}" )

set(benchmarker_SRCS
  maildir/maildir.cpp  
  maildir/maildirimport.cpp  
  main.cpp  
  maketest.cpp  
  test.cpp  
  testmaildir.cpp
  maildir/maildirfetchallheaders.cpp
  maildir/maildir20percentread.cpp
  maildir/maildirfetchunreadheaders.cpp
  maildir/maildirremovereadmessages.cpp

  testvcard.cpp
  vcard/vcard.cpp
  vcard/vcardimport.cpp
)

kde4_add_executable(akonadi_benchmarker ${benchmarker_SRCS})

target_link_libraries(akonadi_benchmarker
  akonadi-kde
  akonadi-kmime
  ${KDE4_KDEUI_LIBS}
  ${KDE4_AKONADI_LIBS}
  ${KDE4_KMIME_LIBRARY}
  ${KDE4_KDECORE_LIBS}
  ${QT_QTCORE_LIBRARY}
  ${QT_QTDBUS_LIBRARY}
  ${QT_QTTEST_LIBRARY}
)

install(TARGETS akonadi_benchmarker ${KF5_INSTALL_TARGETS_DEFAULT_ARGS})
