include(FetchContent)

FetchContent_Declare(
    detours
    SOURCE_DIR ${PROJECT_SOURCE_DIR}/deps/detours
)

FetchContent_MakeAvailable(detours)
