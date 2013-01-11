set( EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR} )
include_directories( ${CMAKE_SOURCE_DIR}/akonadi/contact )

# convenience macro to add akonadi demo application
macro(add_akonadi_contact_demo _source)
  set(_test ${_source})
  get_filename_component(_name ${_source} NAME_WE)
  kde4_add_executable(${_name} TEST ${_test})
  target_link_libraries(${_name} akonadi-kde akonadi-kmime akonadi-contact ${QT_QTCORE_LIBRARY} ${QT_QTGUI_LIBRARY} ${KDE4_KDECORE_LIBS} ${KDE4_KDEUI_LIBS})
endmacro()

########### next target ###############

set(contactmetadataattributetest_SRCS contactmetadataattributetest.cpp)
kde4_add_unit_test(contactmetadataattributetest TESTNAME contactmetadataattributetest ${contactmetadataattributetest_SRCS})
target_link_libraries(contactmetadataattributetest ${QT_QTTEST_LIBRARY} ${QT_QTCORE_LIBRARY} ${KDE4_KDECORE_LIBS} akonadi-contact)

########### next target ###############

add_akonadi_contact_demo(emailaddressselectionwidgettest.cpp)
add_akonadi_contact_demo(emailaddressselectiondialogtest.cpp)
