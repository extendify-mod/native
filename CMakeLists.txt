cmake_minimum_required(VERSION 3.10)

# Project name
project(extendify LANGUAGES CXX C)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU" AND WIN32)

message(FATAL_ERROR "GCC is not supported on Windows. Please use Clang.")

endif()

# Set the source directory
set(SRC_DIR "${CMAKE_SOURCE_DIR}/src")

# Collect all source files in the src directory
file(GLOB_RECURSE SOURCES "${SRC_DIR}/*.cpp" "${SRC_DIR}/*.c")

include(FetchContent)
if(CMAKE_SYSTEM_PROCESSOR MATCHES "AMD64")
    set(CEF_ARCH "64")
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "ARM64")
    set(CEF_ARCH "arm64")
else()
    message(FATAL_ERROR "Unsupported arch: ${CMAKE_SYSTEM_PROCESSOR}")
endif()

if(LINUX)
    set(CEF_OS "linux")
elseif(WIN32)
    set(CEF_OS "windows")
else()
    message(FATAL_ERROR "Unsupported OS")
endif()
# set the download hash
if(WIN32 AND CEF_ARCH STREQUAL "64")
    set(CEF_HASH "fcd3e449bc5dcaca81f3056897c90e432e7042a6f38a854f59b45fa5cf2f36bc")
elseif(WIN32 AND CEF_ARCH STREQUAL "arm64")
    set(CEF_HASH "bd4c72ddb97d80f378b1e2ac803f60ed24387da6d2ce0acf38f088af05128714")
elseif(LINUX AND CEF_ARCH STREQUAL "64")
    set(CEF_HASH "deadbeef")
else()
    set(CEF_HASH "deadbeef")
endif()

set(CEF_DOWNLOAD_URL "https://cef-builds.spotifycdn.com/cef_binary_135.0.22%2Bg442c600%2Bchromium-135.0.7049.115_${CEF_OS}${CEF_ARCH}.tar.bz2")
cmake_policy(SET CMP0169 OLD)
FetchContent_Declare(
    cef
    URL ${CEF_DOWNLOAD_URL}
    URL_HASH SHA256=${CEF_HASH}
    SOURCE_DIR ${PROJECT_SOURCE_DIR}/deps/cef
    PATCH_COMMAND patch ./cmake/cef_variables.cmake "${CMAKE_CURRENT_LIST_DIR}/cmake/clangCl.patch"
    DOWNLOAD_EXTRACT_TIMESTAMP FALSE
)
FetchContent_Populate(cef)

find_package(spdlog REQUIRED)
find_package(cef REQUIRED)
find_package(cef_dll_wrapper REQUIRED)
if(WIN32)
    find_package(detours REQUIRED)
endif()
# Create a shared library
add_library(extendify SHARED ${SOURCES})

# Link the library against spdlog
target_link_libraries(extendify PRIVATE spdlog)
# target_link_libraries(extendify PRIVATE cef::libcef_dll_wrapper)
target_link_libraries(extendify PUBLIC cef::libcef)
if(WIN32)
    target_link_libraries(extendify PRIVATE detours)
endif()

# Specify the include directories if needed
target_include_directories(extendify PRIVATE "${SRC_DIR}")