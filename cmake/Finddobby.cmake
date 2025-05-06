include(FetchContent)

FetchContent_Declare(
        dobby
        SOURCE_DIR ${PROJECT_SOURCE_DIR}/deps/dobby
)

FetchContent_MakeAvailable(dobby)
