macro(add_akonadi_isolated_test _source)
  set(_test ${_source})
  get_filename_component(_name ${_source} NAME_WE)
  kde4_add_executable(${_name} TEST ${_test})
  target_link_libraries(${_name}
    akonadi-kde akonadi-calendar kcalcore ${QT_QTTEST_LIBRARY}
    ${QT_QTGUI_LIBRARY} ${KDE4_KDECORE_LIBS}
  )
 
  # based on kde4_add_unit_test
  if (WIN32)
    get_target_property( _loc ${_name} LOCATION )
    set(_executable ${_loc}.bat)
  else (WIN32)
    set(_executable ./${_name})
  endif (WIN32)
  if (UNIX)
    set(_executable ${_executable}.shell)
  endif (UNIX)
 
  find_program(_testrunner akonaditest)
 
  add_test( libakonadi-${_name} 
    ${_testrunner} -c
    ${CMAKE_CURRENT_SOURCE_DIR}/unittestenv/config.xml
    ${_executable} 
  )
endmacro(add_akonadi_isolated_test)

add_akonadi_isolated_test( incidencechangertest.cpp )
#add_akonadi_isolated_test( calendarbasetest.cpp )
#add_akonadi_isolated_test( fetchjobcalendartest.cpp )
#add_akonadi_isolated_test( etmcalendartest.cpp )
