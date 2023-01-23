find_package(Qt${QT_MAJOR_VERSION}Test ${QT_REQUIRED_VERSION} CONFIG REQUIRED)

if(${EXECUTABLE_OUTPUT_PATH})
    set( PREVIOUS_EXEC_OUTPUT_PATH ${EXECUTABLE_OUTPUT_PATH} )
else()
    set( PREVIOUS_EXEC_OUTPUT_PATH . )
endif()
set( EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR} )
set( TEST_RESULT_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR}/testresults )
file(MAKE_DIRECTORY ${TEST_RESULT_OUTPUT_PATH})

kde_enable_exceptions()

# convenience macro to add akonadi demo application
macro(add_akonadi_demo _source)
  set(_test ${_source})
  get_filename_component(_name ${_source} NAME_WE)
  add_executable(${_name} ${_test})
  target_link_libraries(${_name}
        KF5AkonadiCore
        KF5AkonadiWidgets
        KF${KF_MAJOR_VERSION}::I18n
  )
endmacro()

# demo applications
add_akonadi_demo(itemdumper.cpp itemdumper.h)
add_akonadi_demo(subscriber.cpp)
add_akonadi_demo(agentinstancewidgettest.cpp agentinstancewidgettest.h)
add_akonadi_demo(agenttypewidgettest.cpp agenttypewidgettest.h)
add_akonadi_demo(pluginloadertest.cpp)
##REACTIVATE
#add_akonadi_demo(selftester.cpp)
add_akonadi_demo(collectiondialog.cpp)
add_akonadi_demo(conflictresolvedialogtest_gui.cpp)

add_subdirectory(etm_test_app)
