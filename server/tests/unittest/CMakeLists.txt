########### next target ###############

set(handlertest_SRCS handlertest.cpp teststoragebackend.cpp )

kde4_automoc(${handlertest_SRCS})

# FIXME: handlertest hangs because you can't open the mysql database twice
#kde4_add_unit_test(handlertes TESTNAME akonadi-handlertest ${handlertest_SRCS})

target_link_libraries(handlertest akonadiprivate ${QT_QTTEST_LIBRARY})

