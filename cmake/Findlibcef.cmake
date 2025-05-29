include(FindPackageHandleStandardArgs)
include(FetchContent)
if (CMAKE_SYSTEM_PROCESSOR MATCHES "AMD64|x86_64")
    set(CEF_ARCH "64")
elseif (CMAKE_SYSTEM_PROCESSOR MATCHES "ARM64")
    set(CEF_ARCH "arm64")
else ()
    message(FATAL_ERROR "Unsupported arch: ${CMAKE_SYSTEM_PROCESSOR}")
endif ()

if (LINUX)
    set(CEF_OS "linux")
elseif (WIN32)
    set(CEF_OS "windows")
else ()
    message(FATAL_ERROR "Unsupported OS")
endif ()
# set the download hash
if (WIN32 AND CEF_ARCH STREQUAL "64")
    set(CEF_HASH "5f98646b593702c1d79d637670875eb1e857d64c")
elseif (WIN32 AND CEF_ARCH STREQUAL "arm64")
    set(CEF_HASH "c5e44198b5b855331a752b3a63b29e9bea7e2d85")
elseif (LINUX AND CEF_ARCH STREQUAL "64")
    set(CEF_HASH "b747fe7d45f36978aff3ba8991254a8fbbfbaf66")
else ()
    set(CEF_HASH "")
    message(ERROR "HASH NOT SET FOR ${CEF_OS}/${CEF_ARCH}")
endif ()

find_library(
        libcef
        NAMES cef
)
if (NOT (libcef AND libcef_INCLUDE_DIR))
    # go to https://www.spotify.com/us/opensource/, copy correct cef version, run it in this
    # paste | python -c 'import sys; from urllib.parse import quote; print(quote(sys.stdin.read()))' | tee /dev/stderr | copy
    set(BUILD_STR_VER "134.3.11")
    set(BUILD_STR_REV "7c94248")
    set(BUILD_STR_CHROMIUM "134.0.6998.178")
    set(BUILD_STR "${BUILD_STR_VER}%2Bg${BUILD_STR_REV}%2Bchromium-${BUILD_STR_CHROMIUM}")
    set(CEF_DOWNLOAD_URL "https://cef-builds.spotifycdn.com/cef_binary_${BUILD_STR}_${CEF_OS}${CEF_ARCH}_minimal.tar.bz2")
    cmake_policy(SET CMP0169 OLD)

    FetchContent_Declare(
            cef
            URL ${CEF_DOWNLOAD_URL}
            URL_HASH SHA1=${CEF_HASH}
            DOWNLOAD_EXTRACT_TIMESTAMP FALSE
    )
    FetchContent_Populate(cef)
    set(libcef_FOUND TRUE)
    find_library(libcef NAMES cef HINTS "${cef_SOURCE_DIR}/Release/")
    if(WIN32)
    set(libcef_LIBRARY "${cef_SOURCE_DIR}/Release/libcef.dll")
    set(libcef_IMPLIB "${cef_SOURCE_DIR}/Release/libcef.lib")
    else() 
    set(libcef_LIBRARY "${cef_SOURCE_DIR}/Release/libcef.so")
    endif()
    set(libcef_INCLUDE_DIR "${cef_SOURCE_DIR}/include")
endif ()
find_package_handle_standard_args(libcef REQUIRED_VARS libcef libcef_INCLUDE_DIR)

add_library(libcef SHARED IMPORTED)
set_property(TARGET libcef PROPERTY IMPORTED_LOCATION "${libcef}")
if (WIN32)
    set_property(TARGET libcef PROPERTY IMPORTED_IMPLIB "${libcef_IMPLIB}")
endif()
target_include_directories(libcef INTERFACE "${libcef_INCLUDE_DIR}")
if (NOT EXISTS "${libcef_INCLUDE_DIR}/include")
    file(CREATE_LINK "${libcef_INCLUDE_DIR}" "${libcef_INCLUDE_DIR}/include" SYMBOLIC)
endif ()
