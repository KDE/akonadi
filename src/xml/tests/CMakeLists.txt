
set(QT_REQUIRED_VERSION "5.2.0")
find_package(Qt5 ${QT_REQUIRED_VERSION} CONFIG REQUIRED COMPONENTS Test)

include_directories(
    ${CMAKE_CURRENT_BINARY_DIR}/..
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)

macro(add_libakonadixml_test _source)
  set(_test ${_source})
  get_filename_component(_name ${_source} NAME_WE)
  add_executable( ${_name} ${_test} )
  add_test( ${_name} ${_name} )
  ecm_mark_as_test(akonadixml-${_name})
  target_link_libraries(${_name} KF5::AkonadiXml KF5::AkonadiCore Qt5::Test Qt5::Xml) 
endmacro()

add_libakonadixml_test(collection.cpp)
#REACTIVATE IT add_libakonadixml_test(xmldocumenttest.cpp)
