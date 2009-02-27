
include_directories(
    ${CMAKE_SOURCE_DIR}/libakonadi-xml
    ${CMAKE_SOURCE_DIR}
    ${AKONADI_INCLUDE_DIR}
    ${KDE4_INCLUDES}
    ${KDEPIMLIBS_INCLUDE_DIR}
)

macro(add_libakonadixml_test _source)
  set(_test ${_source})
  get_filename_component(_name ${_source} NAME_WE)
  kde4_add_unit_test(${_name} TESTNAME akonadixml-${_name} ${_test})
  target_link_libraries(${_name} akonadixml ${QT_QTTEST_LIBRARY} ${QT_QTGUI_LIBRARY} ${QT_QTCORE_LIBRARY} ${KDE4_KDEUI_LIBS}
  ${AKONADI_COMMON_LIBRARIES})
endmacro(add_libakonadixml_test)

add_libakonadixml_test(collection.cpp)
