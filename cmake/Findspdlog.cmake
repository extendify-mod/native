include(FetchContent)

FetchContent_Declare(
    spdlog
    SOURCE_DIR ${PROJECT_SOURCE_DIR}/deps/spdlog
)

FetchContent_MakeAvailable(spdlog)