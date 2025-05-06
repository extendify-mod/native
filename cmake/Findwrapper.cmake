include(FetchContent)

FetchContent_Declare(
        extendify_wrapper
        SOURCE_DIR ${PROJECT_SOURCE_DIR}/deps/wrapper
)

FetchContent_MakeAvailable(extendify_wrapper)
