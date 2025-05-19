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
add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/deps/wrapper)

# Link the library against spdlog
target_link_libraries(extendify PRIVATE spdlog)
target_link_libraries(extendify PRIVATE libcef_dll_wrapper)
target_link_libraries(extendify PUBLIC libcef)
if (WIN32)
    target_link_libraries(extendify PRIVATE detours)
elseif (LINUX)
    target_link_libraries(extendify PUBLIC dobby_static)
endif ()

# Specify the include directories if needed
target_include_directories(extendify PRIVATE "${SRC_DIR}")
set_property(TARGET extendify PROPERTY CXX_STANDARD 23)
include(cmake/compilerChecks.cmake)
if (CMAKE_BUILD_TYPE STREQUAL "Release")
elseif (SUPPORTS_GDWARF)
    target_compile_options(extendify PUBLIC -gdwarf)
else ()
    message(WARNING "-gdwarf is not supported, no debug symbols will be output")
endif ()
if(C_GETENV_DEPRECATED)
    message(STATUS "getenv is deprecated, suppressing")
    target_compile_definitions(extendify PRIVATE _CRT_SECURE_NO_WARNINGS)
endif()
if(WIN32_STD_MINMAX_NEEDS_NOMINMAX)
    message(STATUS "std::min and std::max need NOMINMAX")
    target_compile_definitions(extendify PRIVATE NOMINMAX)
endif()

add_custom_target(extendify.copyDeps DEPENDS setdll extendify_wrapper)
add_custom_command(TARGET extendify.copyDeps
        PRE_BUILD
        COMMAND
        ${CMAKE_COMMAND} -E copy $<TARGET_FILE:setdll> ${CMAKE_CURRENT_BINARY_DIR}/setdll.exe
)
add_dependencies(extendify extendify.copyDeps)
add_custom_command(TARGET extendify.copyDeps
        PRE_BUILD
        COMMAND
        ${CMAKE_COMMAND} -E copy $<TARGET_FILE:extendify_wrapper> ${CMAKE_CURRENT_BINARY_DIR}/extendify_wrapper.dll
)
