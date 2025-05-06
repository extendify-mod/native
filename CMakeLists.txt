cmake_minimum_required(VERSION 3.31)

# Project name
project(extendify LANGUAGES CXX C)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)

if (CMAKE_CXX_COMPILER_ID MATCHES "GNU" AND WIN32)

    message(FATAL_ERROR "GCC is not supported on Windows. Please use Clang.")

endif ()

# Set the source directory
set(SRC_DIR "${CMAKE_SOURCE_DIR}/src")

# Collect all source files in the src directory
file(GLOB_RECURSE SOURCES "${SRC_DIR}/*.cpp" "${SRC_DIR}/*.c")

find_package(spdlog REQUIRED)
find_package(libcef REQUIRED)
find_package(cef_dll_wrapper REQUIRED)
if (WIN32)
    find_package(detours REQUIRED)
elseif (LINUX)
    find_package(dobby)
endif ()
# Create a shared library
add_library(extendify SHARED ${SOURCES})

# Link the library against spdlog
target_link_libraries(extendify PRIVATE spdlog)
target_link_libraries(extendify PUBLIC libcef_dll_wrapper)
target_link_libraries(extendify PUBLIC libcef)
if (WIN32)
    target_link_libraries(extendify PRIVATE detours)
elseif (LINUX)
    target_link_libraries(extendify PUBLIC dobby_static)
endif ()

# Specify the include directories if needed
target_include_directories(extendify PRIVATE "${SRC_DIR}")
set_property(TARGET extendify PROPERTY CXX_STANDARD 23)
if (CMAKE_BUILD_TYPE STREQUAL "Release")
else ()
    target_compile_options(extendify PUBLIC -gdwarf)
endif ()