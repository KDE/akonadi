
include_directories(
    ${CMAKE_CURRENT_BINARY_DIR}/..
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${KDE4_INCLUDES}
)

macro(add_libakonadixml_test _source)
  set(_test ${_source})
  get_filename_component(_name ${_source} NAME_WE)
  kde4_add_unit_test(${_name} TESTNAME akonadixml-${_name} ${_test})
  target_link_libraries(${_name} akonadi-xml ${QT_QTTEST_LIBRARY} ${QT_QTXML_LIBRARY}
                        ${KDE4_KDEUI_LIBS} akonadi-kde)
endmacro()

#add_libakonadixml_test(collection.cpp)
#add_libakonadixml_test(xmldocumenttest.cpp)
