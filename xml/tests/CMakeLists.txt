
include_directories(
    ${kdepim-runtime_SOURCE_DIR}
    ${kdepim-runtime_SOURCE_DIR}/xml
    ${AKONADI_INCLUDE_DIR}
    ${KDE4_INCLUDES}
    ${KDEPIMLIBS_INCLUDE_DIR}
)

macro(add_libakonadixml_test _source)
  set(_test ${_source})
  get_filename_component(_name ${_source} NAME_WE)
  kde4_add_unit_test(${_name} TESTNAME akonadixml-${_name} ${_test})
  target_link_libraries(${_name} akonadi-xml ${QT_QTTEST_LIBRARY} ${QT_QTXML_LIBRARY}
                        ${KDE4_KDEUI_LIBS} ${KDEPIMLIBS_AKONADI_LIBS})
endmacro(add_libakonadixml_test)

add_libakonadixml_test(collection.cpp)
add_libakonadixml_test(xmldocumenttest.cpp)
