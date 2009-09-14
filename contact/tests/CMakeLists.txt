set( EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR} )
include_directories( ${CMAKE_SOURCE_DIR}/akonadi/contact )

########### next target ###############

set(contactmetadataattributetest_SRCS contactmetadataattributetest.cpp)
kde4_add_unit_test(contactmetadataattributetest TESTNAME contactmetadataattributetest ${contactmetadataattributetest_SRCS})
target_link_libraries(contactmetadataattributetest ${QT_QTTEST_LIBRARY} ${QT_QTCORE_LIBRARY} ${KDE4_KDECORE_LIBS} akonadi-contact)

