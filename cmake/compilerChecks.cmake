include(CheckCSourceCompiles)
include(CheckCCompilerFlag)
include(CheckCXXCompilerFlag)
function(invertBool var)
    if(${var})
        set(${var} FALSE PARENT_SCOPE)
    else ()
        set(${var} TRUE PARENT_SCOPE)
    endif()
endfunction()

macro(check_c_getenv_deprecated var)
    check_c_source_compiles("
            #include <stdlib.h>
            int main() {
                getenv(\"\");
            }
    " ${var} FAIL_REGEX "'getenv' is deprecated")
    invertBool(${var})
endmacro()
macro(check_supports_gdwarf var)
    check_c_compiler_flag(-gdwarf ${var})
    # check that both c and cxx compilers support it
    if(${var})
        check_cxx_compiler_flag(-gdwarf ${var})
    endif ()
endmacro()
message(STATUS "Starting checks for extendify")
check_c_getenv_deprecated(C_GETENV_DEPRECATED)
check_supports_gdwarf(SUPPORTS_GDWARF)
message(STATUS "Ending checks for extendify")