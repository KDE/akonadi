########### next target ###############

set(handlertest_SRCS handlertest.cpp teststoragebackend.cpp )

kde4_add_unit_test(handlertest TESTNAME akonadi-handlertest ${handlertest_SRCS})

target_link_libraries(handlertest akonadiprivate ${QT_QTTEST_LIBRARY})
