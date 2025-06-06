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

macro(win_check_std__minmax_needs_NOMINMAX var)
    check_cxx_source_compiles("
        #include <minwindef.h>

        #include <algorithm>
        int main() {
            auto a = std::min(0, 1);
            auto b = std::max(0, 1);
            return a + b;
        }
    " ${var})

    invertBool(${var})
    
endmacro(win_check_std__minmax_needs_NOMINMAX)

macro(check_supports_gdwarf var)
    check_c_compiler_flag(-gdwarf ${var})
    # check that both c and cxx compilers support it
    if(${var})
        check_cxx_compiler_flag(-gdwarf ${var})
    endif ()
endmacro()
macro(check_supports_no_missing_designated_field_initializers var)
    check_c_compiler_flag(-Wno-missing-designated-field-initializers ${var})
endmacro()
message(STATUS "Starting checks for extendify")
check_c_getenv_deprecated(C_GETENV_DEPRECATED)
check_supports_gdwarf(SUPPORTS_GDWARF)
check_supports_no_missing_designated_field_initializers(SUPPORTS_NO_MISSING_DESIGNATED_FIELD_INITIALIZERS)
if(WIN32)
    win_check_std__minmax_needs_NOMINMAX(WIN32_STD_MINMAX_NEEDS_NOMINMAX)
else()
    set(WIN32_STD_MINMAX_NEEDS_NOMINMAX FALSE)
endif()
message(STATUS "Ending checks for extendify")