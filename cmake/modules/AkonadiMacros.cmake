
macro(akonadi_generate_schema _schemaXml _className _fileBaseName)
add_custom_command(
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${_fileBaseName}.h
         ${CMAKE_CURRENT_BINARY_DIR}/${_fileBaseName}.cpp
  COMMAND ${XSLTPROC_EXECUTABLE}
          --output ${CMAKE_CURRENT_BINARY_DIR}/${_fileBaseName}.h
          --stringparam code header
          --stringparam className ${_className}
          --stringparam fileName ${_fileBaseName}
          ${Akonadi_SOURCE_DIR}/src/server/storage/schema.xsl
          ${_schemaXml}
  COMMAND ${XSLTPROC_EXECUTABLE}
          --output ${CMAKE_CURRENT_BINARY_DIR}/${_fileBaseName}.cpp
          --stringparam code source
          --stringparam className ${_className}
          --stringparam fileName ${_fileBaseName}
          ${Akonadi_SOURCE_DIR}/src/server/storage/schema.xsl
          ${_schemaXml}
  DEPENDS ${Akonadi_SOURCE_DIR}/src/server/storage/schema.xsl
          ${Akonadi_SOURCE_DIR}/src/server/storage/schema-header.xsl
          ${Akonadi_SOURCE_DIR}/src/server/storage/schema-source.xsl
          ${_schemaXml}
)
endmacro()
