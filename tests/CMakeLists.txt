if(${EXECUTABLE_OUTPUT_PATH})
    set( PREVIOUS_EXEC_OUTPUT_PATH ${EXECUTABLE_OUTPUT_PATH} )
else(${EXECUTABLE_OUTPUT_PATH})
    set( PREVIOUS_EXEC_OUTPUT_PATH . )
endif(${EXECUTABLE_OUTPUT_PATH})
set( EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR} )

set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${KDE4_ENABLE_EXCEPTIONS}" )

include_directories(
    ${CMAKE_SOURCE_DIR}/akonadi
    ${CMAKE_CURRENT_SOURCE_DIR}/../
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}/../
    ${Boost_INCLUDE_DIR}
    ${AKONADI_INCLUDE_DIR}
)

# add testrunner (application for managing a self-contained test
# environment)

add_subdirectory(testrunner)

# add benchmarker
add_subdirectory(benchmarker)

# convenience macro to add akonadi demo application
macro(add_akonadi_demo _source)
  set(_test ${_source})
  get_filename_component(_name ${_source} NAME_WE)
  kde4_add_executable(${_name} TEST ${_test})
  target_link_libraries(${_name} akonadi-kde akonadi-kmime ${QT_QTCORE_LIBRARY} ${QT_QTGUI_LIBRARY} ${KDE4_KDECORE_LIBS} ${KDE4_KDEUI_LIBS})
endmacro(add_akonadi_demo)

# convenience macro to add akonadi qtestlib unit-tests
macro(add_akonadi_test _source)
  set(_test ${_source})
  get_filename_component(_name ${_source} NAME_WE)
  kde4_add_unit_test(${_name} TESTNAME akonadi-${_name} ${_test})
  target_link_libraries(${_name} akonadi-kde akonadi-kmime ${QT_QTTEST_LIBRARY} ${QT_QTCORE_LIBRARY} ${QT_QTGUI_LIBRARY} ${KDE4_KDECORE_LIBS} ${AKONADI_COMMON_LIBRARIES})
endmacro(add_akonadi_test)

# convenience macro to add akonadi testrunner unit-tests
macro(add_akonadi_isolated_test _source)
  set(_test ${_source})
  get_filename_component(_name ${_source} NAME_WE)
  kde4_add_executable(${_name} TEST ${_test})
  target_link_libraries(${_name} akonadi-kde akonadi-kmime ${QT_QTTEST_LIBRARY} ${QT_QTCORE_LIBRARY} ${QT_QTGUI_LIBRARY} ${KDE4_KDECORE_LIBS} ${AKONADI_COMMON_LIBRARIES})

  # based on kde4_add_unit_test
  if (WIN32)
    get_target_property( _loc ${_name} LOCATION )
    set(_executable ${_loc}.bat)
    set(_testrunner ${PREVIOUS_EXEC_OUTPUT_PATH}/akonaditest.bat)
  else (WIN32)
    set(_executable ${EXECUTABLE_OUTPUT_PATH}/${_name})
    set(_testrunner ${PREVIOUS_EXEC_OUTPUT_PATH}/akonaditest)
  endif (WIN32)
  if (UNIX)
    set(_executable ${_executable}.shell)
    set(_testrunner ${_testrunner}.shell)
  endif (UNIX)

  add_test( akonadi-db-${_name} ${_testrunner} -c ${CMAKE_CURRENT_SOURCE_DIR}/unittestenv/config-db.xml ${_executable} )
  add_test( akonadi-fs-${_name} ${_testrunner} -c ${CMAKE_CURRENT_SOURCE_DIR}/unittestenv/config-fs.xml ${_executable} )
  #add_test( akonadi-sqlite-${_name} ${_testrunner} -c ${CMAKE_CURRENT_SOURCE_DIR}/unittestenv/config-sqlite.xml ${_executable} )
endmacro(add_akonadi_isolated_test)


# demo applications
add_akonadi_demo(itemdumper.cpp)
add_akonadi_demo(subscriber.cpp)
add_akonadi_demo(headfetcher.cpp)
add_akonadi_demo(agentinstancewidgettest.cpp)
add_akonadi_demo(agenttypewidgettest.cpp)
add_akonadi_demo(pluginloadertest.cpp)
add_akonadi_demo(selftester.cpp)

kde4_add_executable( akonadi-firstrun TEST ../firstrun.cpp firstrunner.cpp )
target_link_libraries( akonadi-firstrun akonadi-kde ${KDE4_KDEUI_LIBS} )

# qtestlib unit tests
add_akonadi_test(imapparsertest.cpp)
add_akonadi_test(imapsettest.cpp)
add_akonadi_test(itemhydratest.cpp)
add_akonadi_test(itemtest.cpp)
add_akonadi_test(itemserializertest.cpp)
add_akonadi_test(mimetypecheckertest.cpp)
add_akonadi_test(notificationmessagetest.cpp)

# testrunner tests
add_akonadi_isolated_test(autoincrementtest.cpp)
add_akonadi_isolated_test(testenvironmenttest.cpp)
add_akonadi_isolated_test(collectionjobtest.cpp)
add_akonadi_isolated_test(collectionpathresolvertest.cpp)
add_akonadi_isolated_test(collectionattributetest.cpp)
add_akonadi_isolated_test(itemfetchtest.cpp)
add_akonadi_isolated_test(itemappendtest.cpp)
add_akonadi_isolated_test(itemstoretest.cpp)
add_akonadi_isolated_test(itemdeletetest.cpp)
add_akonadi_isolated_test(monitortest.cpp)
add_akonadi_isolated_test(searchjobtest.cpp)
add_akonadi_isolated_test(changerecordertest.cpp)
add_akonadi_isolated_test(resourcetest.cpp)
add_akonadi_isolated_test(subscriptiontest.cpp)
add_akonadi_isolated_test(transactiontest.cpp)
add_akonadi_isolated_test(itemcopytest.cpp)
add_akonadi_isolated_test(itemmovetest.cpp)
add_akonadi_isolated_test(collectioncopytest.cpp)
add_akonadi_isolated_test(collectionmovetest.cpp)
add_akonadi_isolated_test(itemsynctest.cpp)
add_akonadi_isolated_test(linktest.cpp)
add_akonadi_isolated_test(cachetest.cpp)
add_akonadi_isolated_test(servermanagertest.cpp)
add_akonadi_isolated_test(collectioncreator.cpp)
