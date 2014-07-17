project (benchmarker)

include_directories(
    ${CMAKE_SOURCE_DIR}/akonadi
    ${CMAKE_BINARY_DIR}/akonadi
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${Boost_INCLUDE_DIR}
    ${AKONADI_INCLUDE_DIR}
)

kde_enable_exceptions()

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

add_executable(akonadi_benchmarker ${benchmarker_SRCS})

target_link_libraries(akonadi_benchmarker
  akonadi-kde
  KF5::AkonadiMime
  ${KDE4_AKONADI_LIBS}
  KF5::Mime
  Qt5::Test
)

install(TARGETS akonadi_benchmarker ${KF5_INSTALL_TARGETS_DEFAULT_ARGS})
