
if(KDE4_BUILD_TESTS)

include_directories(
	${CMAKE_SOURCE_DIR}/akonadi
	${CMAKE_CURRENT_SOURCE_DIR}/../
	${CMAKE_CURRENT_SOURCE_DIR}
	${CMAKE_SOURCE_DIR}/libkmime
)

# convenience macro to add akonadi demo application
macro(add_akonadi_demo _source)
  set(_test ${_source})
  get_filename_component(_name ${_source} NAME_WE)
  kde4_automoc(${_name} ${_test})
  kde4_add_executable(${_name} RUN_UNINSTALLED ${_test})
  target_link_libraries(${_name} akonadi)
endmacro(add_akonadi_demo)


# convenience macro to add akonadi qtestlib unit-tests
macro(add_akonadi_test _source)
  set(_test ${_source})
  get_filename_component(_name ${_source} NAME_WE)
  kde4_automoc(${_name} ${_test})
  kde4_add_executable(${_name} RUN_UNINSTALLED ${_test})
  target_link_libraries(${_name} akonadi ${QT_QTTEST_LIBRARY})
  add_test( libakonadi-${_name} ${EXECUTABLE_OUTPUT_PATH}/${_name})
endmacro(add_akonadi_test)

########### next target ###############

set(collectionbrowser_SRCS collectionbrowser.cpp messagebrowser.cpp )

kde4_automoc(collectionbrowser ${collectionbrowser_SRCS})

kde4_add_executable(collectionbrowser RUN_UNINSTALLED ${collectionbrowser_SRCS})

target_link_libraries(collectionbrowser  akonadi ${KDE4_KDECORE_LIBS} )

########### next target ###############

set(messagebrowser_SRCS messagebrowser.cpp  messagebrowser_main.cpp )

kde4_automoc(messagebrowser ${messagebrowser_SRCS})

kde4_add_executable(messagebrowser RUN_UNINSTALLED ${messagebrowser_SRCS})

target_link_libraries(messagebrowser  akonadi ${KDE4_KDECORE_LIBS} )


# demo applications
add_akonadi_demo(itemdumper.cpp)
add_akonadi_demo(collectioncreator.cpp)

# qtestlib unit tests
add_akonadi_test(imapparsertest.cpp)
add_akonadi_test(collectionjobtest.cpp)
add_akonadi_test(messagejobtest.cpp)
add_akonadi_test(itemappendtest.cpp)
add_akonadi_test(itemstoretest.cpp)
add_akonadi_test(monitortest.cpp)


endif(KDE4_BUILD_TESTS)

