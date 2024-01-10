
find_package(Qt6 ${QT_REQUIRED_VERSION} CONFIG REQUIRED COMPONENTS Test)
include(ECMMarkAsTest)

macro(add_libakonadixml_test _source)
  set(_test ${_source})
  get_filename_component(_name ${_source} NAME_WE)
  ecm_add_test(TEST_NAME ${_name} NAME_PREFIX "akonadixml-" ${_test})
  set_tests_properties(akonadixml-${_name} PROPERTIES ENVIRONMENT "QT_HASH_SEED=0;QT_NO_CPU_FEATURE=sse4.2")
  set_target_properties(${_name} PROPERTIES COMPILE_FLAGS -DKDESRCDIR="\\"${CMAKE_CURRENT_SOURCE_DIR}/\\"")
  target_link_libraries(${_name} KPim6::AkonadiCore KPim6::AkonadiXml Qt::Test Qt::Xml)
endmacro()

add_libakonadixml_test(collectiontest.cpp collectiontest.h)
add_libakonadixml_test(xmldocumenttest.cpp)
