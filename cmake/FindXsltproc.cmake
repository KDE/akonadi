# Find xsltproc executable and provide a macro to generate D-Bus interfaces.
#
# The following variables are defined :
# XSLTPROC_EXECUTABLE - path to the xsltproc executable
# Xsltproc_FOUND - true if the program was found
#
find_program(XSLTPROC_EXECUTABLE xsltproc DOC "Path to the xsltproc executable")
mark_as_advanced(XSLTPROC_EXECUTABLE)

if(XSLTPROC_EXECUTABLE)
  set(Xsltproc_FOUND TRUE)

  # Macro to generate a D-Bus interface description from a KConfigXT file
  macro(kcfg_generate_dbus_interface _kcfg _name)
    add_custom_command(
      OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${_name}.xml
      COMMAND ${XSLTPROC_EXECUTABLE} --stringparam interfaceName ${_name}
      ${CMAKE_SOURCE_DIR}/akonadi/kcfg2dbus.xsl
      ${_kcfg}
      > ${CMAKE_CURRENT_BINARY_DIR}/${_name}.xml
      DEPENDS ${CMAKE_SOURCE_DIR}/akonadi/kcfg2dbus.xsl
      ${_kcfg}
      )
  endmacro()
endif()

