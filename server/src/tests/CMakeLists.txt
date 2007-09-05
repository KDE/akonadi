########### next target ###############

set(interpretertest_SRCS interpretertest.cpp ../interpreter.cpp )


kde4_add_unit_test(interpretertest TESTNAME akonadi-interpretertest ${interpretertest_SRCS})

target_link_libraries(interpretertest ${KDE4_KDECORE_LIBS} ${QT_QTTEST_LIBRARY})


kde4_add_unit_test(querybuildertest TESTNAME akonadi-querybuildertest querybuildertest.cpp)
target_link_libraries(querybuildertest ${KDE4_KDECORE_LIBS} ${QT_QTSQL_LIBRARY} ${QT_QTTEST_LIBRARY})
