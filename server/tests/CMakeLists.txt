# QTEST_MAIN is using QApplication when QT_GUI_LIB is defined
remove_definitions(-DQT_GUI_LIB)
add_subdirectory( unittest )
