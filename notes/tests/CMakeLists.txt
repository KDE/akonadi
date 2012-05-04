set(notestest_SRCS notestest.cpp)

set(CMAKE_PREFIX_PATH ../)

kde4_add_unit_test(notestest TESTNAME notestest ${notestest_SRCS})
target_link_libraries(notestest ${KDE4_KDECORE_LIBS} akonadi-notes kmime ${QT_QTTEST_LIBRARY})