include(FindPackageHandleStandardArgs)
include(ExternalProject)
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
    set(CEF_HASH "fcd3e449bc5dcaca81f3056897c90e432e7042a6f38a854f59b45fa5cf2f36bc")
elseif (WIN32 AND CEF_ARCH STREQUAL "arm64")
    set(CEF_HASH "bd4c72ddb97d80f378b1e2ac803f60ed24387da6d2ce0acf38f088af05128714")
elseif (LINUX AND CEF_ARCH STREQUAL "64")
    set(CEF_HASH "36a0f07c91c43c68a937e766af3b5bc1a0828bb11656cafa9ed49a4dd6c0bae2")
else ()
    set(CEF_HASH "")
    message(ERROR "HASH NOT SET FOR ${CEF_OS}/${CEF_ARCH}")
endif ()

find_library(
        cef_dll_wrapper_LIBRARY
        NAMES cef_dll_wrapper
)
if (NOT cef_dll_wrapper_FOUND)
    set(CEF_DOWNLOAD_URL "https://cef-builds.spotifycdn.com/cef_binary_135.0.22%2Bg442c600%2Bchromium-135.0.7049.115_${CEF_OS}${CEF_ARCH}.tar.bz2")
    if (LINUX)
        FetchContent_declare(
                cef_dll_wrapper
                URL ${CEF_DOWNLOAD_URL}
                URL_HASH SHA256=${CEF_HASH}
                DOWNLOAD_EXTRACT_TIMESTAMP FALSE
                PATCH_COMMAND patch ./CMakeLists.txt "${CMAKE_CURRENT_LIST_DIR}/noTests.patch"
        )
        fetchcontent_makeavailable(cef_dll_wrapper)
    elseif (WIN32)
        cmake_policy(SET CMP0169 OLD)

        FetchContent_Declare(
                cef_dll_wrapper
                URL ${CEF_DOWNLOAD_URL}
                URL_HASH SHA256=${CEF_HASH}
                PATCH_COMMAND patch ./cmake/cef_variables.cmake "${CMAKE_CURRENT_LIST_DIR}/clangCl.patch"
                DOWNLOAD_EXTRACT_TIMESTAMP FALSE
        )
        FetchContent_Populate(cef_dll_wrapper)
        if (CMAKE_GENERATOR MATCHES "Ninja")
            set(CEF_BUILD_CMD ninja -C ${cef_dll_wrapper_BINARY_DIR} libcef_dll_wrapper)
            set(CEF_GENERATOR Ninja)
        elseif (CMAKE_GENERATOR MATCHES "Unix Makefiles")
            set(CEF_BUILD_CMD make -j -C ${cef_dll_wrapper_BINARY_DIR} libcef_dll_wrapper)
            set(CEF_GENERATOR "Unix Makefiles")
        else ()
            message(ERROR "Only make and Ninja are supported, defaulting to Makefiles")
            set(CEF_BUILD_CMD make -j -C ${cef_dll_wrapper_BINARY_DIR} libcef_dll_wrapper)
            set(CEF_GENERATOR "Unix Makefiles")
        endif ()
        ExternalProject_Add(
                cef_dll_wrapper
                SOURCE_DIR ${cef_dll_wrapper_SOURCE_DIR}
                DOWNLOAD_EXTRACT_TIMESTAMP FALSE
                CONFIGURE_COMMAND cmake -G${CEF_GENERATOR} -B ${cef_dll_wrapper_BINARY_DIR} -DCMAKE_CXX_COMPILER=clang-cl -DCMAKE_C_COMPILER=clang-cl -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE} -DCEF_RUNTIME_LIBRARY_FLAG=/MD
                BUILD_COMMAND ${CEF_BUILD_CMD}
                BUILD_IN_SOURCE TRUE
                INSTALL_COMMAND ""
                BUILD_BYPRODUCTS "${cef_dll_wrapper_BINARY_DIR}/libcef_dll_wrapper/libcef_dll_wrapper.lib"
        )
        set(cef_dll_wrapper_FOUND TRUE)
        set(cef_dll_wrapper_LIBRARY "${cef_dll_wrapper_BINARY_DIR}/libcef_dll_wrapper/libcef_dll_wrapper.lib")
    endif ()
endif ()
find_package_handle_standard_args(cef_dll_wrapper REQUIRED_VARS cef_dll_wrapper_LIBRARY)

add_library(libcef_dll_wrapper STATIC IMPORTED)
set_property(TARGET libcef_dll_wrapper PROPERTY IMPORTED_LOCATION "${cef_dll_wrapper_LIBRARY}")
