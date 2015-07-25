# This functions returns an absolute path to a C++ stdlib header file names
# @includeName@
function(findStdlibInclude includeName filePath)
    # Create the simplest possible valid C++ program - we don't care about
    # what the program does, we only need to feed some valid code to the compiler
    file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/empty_test.cpp"
        "int main() { return 0; }"
    )
    # With the "-v" flag, compiler will print out list of default include paths
    # (i.e. stdlib include paths)
    execute_process(COMMAND ${CMAKE_CXX_COMPILER} -Wp,-v -E ${CMAKE_CURRENT_BINARY_DIR}/empty_test.cpp
                    OUTPUT_QUIET
                    ERROR_VARIABLE compilerSearchPaths
    )
    # Turn the output to a CMake lists (\n -> ;)
    STRING(REPLACE ";" "\\\;" compilerSearchPaths "${compilerSearchPaths}")
    STRING(REPLACE "\n" ";" compilerSearchPaths "${compilerSearchPaths}")
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
                message(STATUS "Found C++ stdlib ${includeName} include file: ${std_file}")
                set(${filePath} "${std_file}" PARENT_SCOPE)
                return()
            endif()
        endif()
    endforeach()

    set(${filePath} "" PARENT_SCOPE)
endfunction()
