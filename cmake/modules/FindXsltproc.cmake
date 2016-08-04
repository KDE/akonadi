# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#
# 1. Redistributions of source code must retain the copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
# 3. The name of the author may not be used to endorse or promote products 
#    derived from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
# IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
# OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
# IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
# INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
# NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
# THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#
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

