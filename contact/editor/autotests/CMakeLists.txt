set(emaileditdialogtesttest_SRCS emaileditdialogtest.cpp ../emaileditwidget.cpp)
kde4_add_unit_test(emaileditdialogtest TESTNAME emaileditdialogtest ${emaileditdialogtesttest_SRCS})
target_link_libraries(emaileditdialogtest ${QT_QTTEST_LIBRARY} ${KDE4_KDEUI_LIBS} kpimutils kabc)

