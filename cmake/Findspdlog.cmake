include(FetchContent)

if (NOT WIN32)
    set(SPDLOG_BUILD_PIC ON)
endif ()
FetchContent_Declare(
        spdlog
        SOURCE_DIR ${PROJECT_SOURCE_DIR}/deps/spdlog
)
set(SPDLOG_USE_STD_FORMAT ON)

FetchContent_MakeAvailable(spdlog)
# needed because libcef_dll_wrapper disables it for sandbox
target_compile_definitions(spdlog PUBLIC _HAS_ITERATOR_DEBUGGING=0)