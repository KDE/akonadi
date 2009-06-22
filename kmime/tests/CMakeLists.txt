set( EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR} )

# TODO apparently QTEST_AKONADIMAIN doesn't like NO_CASTs...
remove_definitions( -DQT_NO_CAST_FROM_ASCII )
remove_definitions( -DQT_NO_CAST_TO_ASCII )

set( requester_srcs foldersrequester.cpp )
kde4_add_executable( requester TEST ${requester_srcs} )
target_link_libraries( requester akonadi-kmime )
# for racetest
set( requester_exe "QLatin1String( \"${CMAKE_CURRENT_BINARY_DIR}/requester\" )" )
add_definitions( -DREQUESTER_EXE='${requester_exe}' )




macro(add_akonadi_isolated_test _source)
  get_filename_component(_targetName ${_source} NAME_WE)
  set(_srcList ${_source} )

  kde4_add_executable(${_targetName} TEST ${_srcList})
  target_link_libraries(${_targetName}
    ${QT_QTTEST_LIBRARY}
    ${QT_QTGUI_LIBRARY}
    ${KDE4_AKONADI_LIBS}
    ${KDE4_KDECORE_LIBS}
    ${KDE4_MAILTRANSPORT_LIBS}
    ${KDE4_KMIME_LIBS}
    ${QT_QTCORE_LIBRARY}
    ${QT_QTDBUS_LIBRARY}
    akonadi-kmime
  )

  # based on kde4_add_unit_test
  if (WIN32)
    get_target_property( _loc ${_targetName} LOCATION )
    set(_executable ${_loc}.bat)
  else (WIN32)
    set(_executable ${EXECUTABLE_OUTPUT_PATH}/${_targetName})
  endif (WIN32)
  if (UNIX)
    set(_executable ${_executable}.shell)
  endif (UNIX)

  find_program(_testrunner akonaditest)

  add_test( akonadikmime-${_targetName} ${_testrunner} -c ${CMAKE_CURRENT_SOURCE_DIR}/unittestenv/config.xml ${_executable} )
endmacro(add_akonadi_isolated_test)




add_akonadi_isolated_test( racetest.cpp )

