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

# Internal server macros
function(akonadi_run_xsltproc)
    if (NOT XSLTPROC_EXECUTABLE)
        message(FATAL_ERROR "xsltproc executable not found but needed by AKONADI_RUN_XSLTPROC()")
    endif()

    set(options )
    set(oneValueArgs XSL XML CLASSNAME BASENAME)
    set(multiValueArgs )
    cmake_parse_arguments(XSLT "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})
    if (NOT XSLT_XSL)
        message(FATAL_ERROR "Required argument XSL missing in AKONADI_RUN_XSLTPROC() call")
    endif()
    if (NOT XSLT_XML)
        message(FATAL_ERROR "Required argument XML missing in AKONADI_RUN_XSLTPROC() call")
    endif()
    if (NOT XSLT_BASENAME)
        message(FATAL_ERROR "Required argument BASENAME missing in AKONADI_RUN_XSLTPROC() call")
    endif()

    # Workaround xsltproc struggling with spaces in filepaths on Windows
    file(RELATIVE_PATH xsl_relpath ${CMAKE_CURRENT_BINARY_DIR} ${XSLT_XSL})
    file(RELATIVE_PATH xml_relpath ${CMAKE_CURRENT_BINARY_DIR} ${XSLT_XML})

    set(extra_params )
    if (XSLT_CLASSNAME)
        set(extra_params --stringparam className ${XSLT_CLASSNAME})
    endif()

    add_custom_command(
        OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${XSLT_BASENAME}.h
            ${CMAKE_CURRENT_BINARY_DIR}/${XSLT_BASENAME}.cpp
        COMMAND ${XSLTPROC_EXECUTABLE}
            --output ${XSLT_BASENAME}.h
            --stringparam code header
            --stringparam fileName ${XSLT_BASENAME}
            ${extra_params}
            ${xsl_relpath}
            ${xml_relpath}
        COMMAND ${XSLTPROC_EXECUTABLE}
            --output ${XSLT_BASENAME}.cpp
            --stringparam code source
            --stringparam fileName ${XSLT_BASENAME}
            ${extra_params}
            ${xsl_relpath}
            ${xml_relpath}
        DEPENDS ${XSLT_XSL}
            ${XSLT_XML}
    )

    set_property(SOURCE
        ${CMAKE_CURRENT_BINARY_DIR}/${XSLT_BASENAME}.cpp
        ${CMAKE_CURRENT_BINARY_DIR}/${XSLT_BASENAME}.h
        PROPERTY SKIP_AUTOMOC TRUE
    )
endfunction()

macro(akonadi_generate_schema _schemaXml _className _fileBaseName)
    if (NOT XSLTPROC_EXECUTABLE)
        message(FATAL_ERROR "xsltproc executable not found but needed by AKONADI_GENERATE_SCHEMA()")
    endif()

    akonadi_run_xsltproc(
        XSL ${Akonadi_SOURCE_DIR}/src/server/storage/schema.xsl
        XML ${_schemaXml}
        CLASSNAME ${_className}
        BASENAME ${_fileBaseName}
    )
endmacro()

function(akonadi_add_xmllint_test)
    if (NOT XMLLINT_EXECUTABLE)
        message(FATAL_ERROR "xmllint executable not found but needed by AKONADI_ADD_XMLLINT_SCHEMA()")
    endif()

    set(options )
    set(oneValueArgs XML XSD)
    set(multiValueArgs )
    cmake_parse_arguments(TEST "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    file(RELATIVE_PATH xsd_relpath ${CMAKE_CURRENT_BINARY_DIR} ${TEST_XSD})
    file(RELATIVE_PATH xml_relpath ${CMAKE_CURRENT_BINARY_DIR} ${TEST_XML})
    add_test(${TEST_UNPARSED_ARGUMENTS} ${XMLLINT_EXECUTABLE} --noout --schema ${xsd_relpath} ${xml_relpath})
endfunction()
