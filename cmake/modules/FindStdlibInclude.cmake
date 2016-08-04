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
# This functions returns an absolute path to a C++ stdlib header file names
# @includeName@
function(findStdlibInclude includeName filePath)
    # Create the simplest possible valid C++ program - we don't care about
    # what the program does, we only need to feed some valid code to the compiler
    file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/empty_test.cpp"
        "#include <${includeName}>\nint main() { return 0; }"
    )
    # With the "-v" flag, compiler will print out list of default include paths
    # (i.e. stdlib include paths)
    if (MSVC)
        execute_process(COMMAND ${CMAKE_CXX_COMPILER} /showIncludes ${CMAKE_CURRENT_BINARY_DIR}/empty_test.cpp
                        OUTPUT_VARIABLE compilerSearchPaths
                        ERROR_QUIET
        )
    else()
        # Handle case when CXX="ccache g++". There is a bug in cmake that causes it
        # to parse ${CMAKE_CXX_COMPILER_ARG1} as " g++" (including the space). We can't
        # then do ${CMAKE_CXX_COMPILER} ${CMAKE_CXX_COMPILER_ARG1}, because CMake will
        # then complain that "ccache  g++" (two spaces) does not exist. We can't do
        # $CMAKE_CXX_COMPILER}${CMAKE_CXX_COMPILER_ARG1} either, because CMake would
        # interpret that as a single argument and would try to exec ("ccache g++" executable
        # instead of "ccache" with "g++" as an argument.
        string(REPLACE " " "" cxx_arg "${CMAKE_CXX_COMPILER_ARG1}")
        execute_process(COMMAND ${CMAKE_CXX_COMPILER} ${cxx_arg} ${CXX_STDLIB_FLAGS} -Wp,-v -E ${CMAKE_CURRENT_BINARY_DIR}/empty_test.cpp
                        OUTPUT_QUIET
                        ERROR_VARIABLE compilerSearchPaths
        )
    endif()

    function(parseOutput output filePath)
        if (MSVC)
            LIST(GET 0 ${compilerSearchPaths} rawline)
            STRING(SUBSTRING "${rawline}" 0 21 lineStart)
            if ("${lineStart}" STREQUAL "Note: including file:")
                STRING(SUBSTRING ${rawline} 22 -1 path)
                get_filename_component(path "${path}" ABSOLUTE)
                set(${filePath} ${path} PARENT_SCOPE)
                return()
            endif()
        else()
            foreach(rawline ${compilerSearchPaths})
                STRING(SUBSTRING "${rawline}" 0 2 lineStart)
                # The output from compiler contains bunch of other things, but the include
                # paths all begin with one whitespace and then path separator
                # FIXME: This probably won't work on Windows...?
                if ("${lineStart}" STREQUAL " /")
                    # Get the actual path (without the opening whitespace)
                    STRING(SUBSTRING ${rawline} 1 -1 path)
                    # The path is often relative, make it absolute without ".."
                    get_filename_component(path "${path}" ABSOLUTE)
                    set(std_file "std_file-NOTFOUND") # ensure it will be looked-up every time
                    find_file(std_file ${includeName} PATHS ${path})
                    if (NOT "${std_file}" STREQUAL "std_file-NOTFOUND")
                        set(${filePath} "${std_file}" PARENT_SCOPE)
                        return()
                    endif()
                endif()
            endforeach()
        endif()
    endfunction()

    # Turn the output to a CMake lists (\n -> ;)
    STRING(REPLACE ";" "\\\;" compilerSearchPaths "${compilerSearchPaths}")
    STRING(REPLACE "\n" ";" compilerSearchPaths "${compilerSearchPaths}")
    set(path "")
    parseOutput(${compilerSearchPaths} path)
    find_file(std_file ${includeName} PATHS ${path})
    if (NOT "${std_file}" STREQUAL "std_file-NOTFOUND")
        message(STATUS "Found C++ stdlib ${includeName} include file: ${std_file}")
        set(${filePath} "${std_file}" PARENT_SCOPE)
        return()
    else()
        message(FATAL_ERROR "Could not find stdlib ${includeName} include file")
        set(${filePath} "" PARENT_SCOPE)
        return()
    endif()
endfunction()
