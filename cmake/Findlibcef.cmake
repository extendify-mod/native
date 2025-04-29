include(FindPackageHandleStandardArgs)
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

find_library(
    libcef_LIBRARY
    NAMES cef
)
if(NOT libcef_FOUND)
    set(CEF_DOWNLOAD_URL "https://cef-builds.spotifycdn.com/cef_binary_135.0.22%2Bg442c600%2Bchromium-135.0.7049.115_${CEF_OS}${CEF_ARCH}.tar.bz2")
    cmake_policy(SET CMP0169 OLD)

    FetchContent_Declare(
        cef
        URL ${CEF_DOWNLOAD_URL}
        URL_HASH SHA256=${CEF_HASH}
        DOWNLOAD_EXTRACT_TIMESTAMP FALSE
    )
    FetchContent_Populate(cef)
    set(libcef_FOUND TRUE)
    set(libcef_LIBRARY "${cef_SOURCE_DIR}/Release/libcef.dll")
    set(libcef_IMPLIB "${cef_SOURCE_DIR}/Release/libcef.lib")
    set(libcef_INCLUDE_DIR "${cef_SOURCE_DIR}/include")
endif()
find_package_handle_standard_args(libcef REQUIRED_VARS libcef_LIBRARY libcef_INCLUDE_DIR)
mark_as_advanced(libcef_LIBRARY)
mark_as_advanced(libcef_INCLUDE_DIR)
mark_as_advanced(libcef_IMPLIB)

add_library(libcef::libcef SHARED IMPORTED)
set_property(TARGET libcef::libcef PROPERTY IMPORTED_LOCATION "${libcef_LIBRARY}")
set_property(TARGET libcef::libcef PROPERTY IMPORTED_IMPLIB "${libcef_IMPLIB}")
target_include_directories(libcef::libcef  INTERFACE $<INSTALL_INTERFACE:"${libcef_INCLUDE_DIR}">)
