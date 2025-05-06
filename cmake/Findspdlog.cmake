include(FetchContent)

if (NOT WIN32)
    set(SPDLOG_BUILD_PIC ON)
endif ()
FetchContent_Declare(
        spdlog
        SOURCE_DIR ${PROJECT_SOURCE_DIR}/deps/spdlog
)

FetchContent_MakeAvailable(spdlog)