if(KDE4_BUILD_TESTS)

# Tests don't need to go into toplevel/bin, they are fine in the current dir.
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR} )

########### next target ###############

set(handlertest_SRCS handlertest.cpp teststoragebackend.cpp )

kde4_automoc(${handlertest_SRCS})

kde4_add_executable(handlertest NOGUI RUN_UNINSTALLED ${handlertest_SRCS})

target_link_libraries(handlertest ${CMAKE_CURRENT_BINARY_DIR}/lib/akonadiprivate ${QT_QTTEST_LIBRARY})

add_test(akonadi-handlertest ${EXECUTABLE_OUTPUT_PATH}/handlertest)

endif(KDE4_BUILD_TESTS)
