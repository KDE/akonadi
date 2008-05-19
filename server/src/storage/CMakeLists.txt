find_program( XMLLINT_EXECUTABLE xmllint )

if ( NOT XMLLINT_EXECUTABLE )
  message( STATUS "xmllint not found, skipping akonadidb.xml schema validation" )
else ( NOT XMLLINT_EXECUTABLE )
  add_test( akonadidb-xmllint ${XMLLINT_EXECUTABLE} --noout --schema ${CMAKE_CURRENT_SOURCE_DIR}/akonadidb.xsd ${CMAKE_CURRENT_SOURCE_DIR}/akonadidb.xml )
  add_test( akonadidbupdate-xmllint ${XMLLINT_EXECUTABLE} --noout --schema ${CMAKE_CURRENT_SOURCE_DIR}/dbupdate.xsd ${CMAKE_CURRENT_SOURCE_DIR}/dbupdate.xml )
endif ( NOT XMLLINT_EXECUTABLE )
