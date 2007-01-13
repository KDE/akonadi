if(KDE4_BUILD_TESTS)

# Tests don't need to go into toplevel/bin, they are fine in the current dir.
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR} )

########### next target ###############

set(interpretertest_SRCS interpretertest.cpp ../interpreter.cpp )

kde4_automoc(${interpretertest_SRCS})

kde4_add_executable(interpretertest NOGUI RUN_UNINSTALLED ${interpretertest_SRCS})

target_link_libraries(interpretertest ${KDE4_KDECORE_LIBS} ${QT_QTTEST_LIBRARY})

add_test(akonadi-interpretertest ${EXECUTABLE_OUTPUT_PATH}/interpretertest)

endif(KDE4_BUILD_TESTS)
