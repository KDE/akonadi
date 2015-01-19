set(emaileditdialogtest_SRCS emaileditdialogtest.cpp ../emaileditwidget.cpp)
kde4_add_unit_test(emaileditdialogtest TESTNAME emaileditdialogtest ${emaileditdialogtest_SRCS})
target_link_libraries(emaileditdialogtest ${QT_QTTEST_LIBRARY} ${KDE4_KDEUI_LIBS} kpimutils kabc)

set(emaileditwidgettest_SRCS emaileditwidgettest.cpp ../emaileditwidget.cpp)
kde4_add_unit_test(emaileditwidgettest TESTNAME emaileditwidgettest ${emaileditwidgettest_SRCS})
target_link_libraries(emaileditwidgettest ${QT_QTTEST_LIBRARY} ${KDE4_KDEUI_LIBS} kpimutils kabc)


