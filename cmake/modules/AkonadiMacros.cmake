# SPDX-License-Identifier: BSD-3-Clause

# Internal server macros
function(akonadi_run_xsltproc)
    if (NOT XSLTPROC_EXECUTABLE)
        message(FATAL_ERROR "xsltproc executable not found but needed by AKONADI_RUN_XSLTPROC()")
    endif()

    set(options )
    set(oneValueArgs XSL XML CLASSNAME BASENAME)
    set(multiValueArgs DEPENDS)
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
            ${XSLT_DEPENDS}
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

function(akonadi_generate_headers_with_deprecated camelcase_forwarding_headers_var)
    # extend as needed to support those ecm_generate_headers arguments akonadi cmake code does use
    set(options)
    set(oneValueArgs PREFIX REQUIRED_HEADERS DEPRECATED_DESTINATION RELATIVE)
    set(multiValueArgs HEADER_NAMES)
    cmake_parse_arguments(ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    if (ARG_UNPARSED_ARGUMENTS)
        message(FATAL_ERROR "Unexpected argument to akonadi_generate_headers: ${ARG_UNPARSED_ARGUMENTS}")
    endif()
    if (NOT ARG_DEPRECATED_DESTINATION)
        message(FATAL_ERROR "DEPRECATED_DESTINATION argument is required with akonadi_generate_headers")
    endif()

    # forward for normal macro
    set(ecmOneValueArgL)
    foreach(ecmOneValueArg PREFIX REQUIRED_HEADERS RELATIVE)
        if (ARG_${ecmOneValueArg} )
            list(APPEND ecmOneValueArgL ${ecmOneValueArg} ${ARG_${ecmOneValueArg}})
        endif()
    endforeach()
    ecm_generate_headers(${camelcase_forwarding_headers_var}
        ${ecmOneValueArgL}
        HEADER_NAMES ${ARG_HEADER_NAMES}
    )
    set(${camelcase_forwarding_headers_var} ${${camelcase_forwarding_headers_var}} PARENT_SCOPE)
    if (ARG_REQUIRED_HEADERS)
        set(${ARG_REQUIRED_HEADERS} ${${ARG_REQUIRED_HEADERS}} PARENT_SCOPE)
    endif ()

endfunction()

