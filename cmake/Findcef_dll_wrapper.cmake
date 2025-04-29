include(FindPackageHandleStandardArgs)
include(ExternalProject)

if(CMAKE_GENERATOR MATCHES "Ninja") 
    set(CEF_BUILD_CMD "ninja -C build libcef_dll_wrapper")
    set(CEF_GENERATOR "Ninja")
elseif(CMAKE_GENERATOR MATCHES "Unix Makefiles")
    set(CEF_BUILD_CMD "make -j -C build libcef_dll_wrapper")
    set(CEF_GENERATOR "Unix Makefiles")
else()
    message(FATAL_ERROR "Only make and Ninja are supported")
endif()

ExternalProject_Add(
    libcef_dll_wrapper
    SOURCE_DIR ${PROJECT_SOURCE_DIR}/deps/cef
    DOWNLOAD_EXTRACT_TIMESTAMP FALSE
    CONFIGURE_COMMAND cmake -GNinja -B build -DCMAKE_CXX_COMPILER=clang-cl -DCMAKE_C_COMPILER=clang-cl
    CMAKE_ARGS -GNinja -DCMAKE_CXX_COMPILER=clang-cl -DCMAKE_C_COMPILER=clang-cl
    # CMAKE_ARGS "-B build -DCMAKE_C_COMPILER=clang-cl -DCMAKE_CXX_COMPILER=clang-cl"
    BUILD_COMMAND ninja -C build libcef_dll_wrapper
    BUILD_IN_SOURCE TRUE
    INSTALL_COMMAND ""
)
find_library(
    cef_dll_wrapper_LIBRARY
    NAMES cef_dll_wrapper
)
if(NOT cef_dll_wrapper_FOUND)
    set(cef_dll_wrapper_FOUND TRUE)
    set(cef_dll_wrapper_LIBRARY "${PROJECT_SOURCE_DIR}/deps/cef/build/libcef_dll_wrapper/libcef_dll_wrapper.lib")
    set(cef_dll_wrapper_INCLUDE_DIR "${PROJECT_SOURCE_DIR}/deps/cef/include")
endif()
find_package_handle_standard_args(cef_dll_wrapper REQUIRED_VARS cef_dll_wrapper_LIBRARY cef_dll_wrapper_INCLUDE_DIR)
mark_as_advanced(cef_dll_wrapper_LIBRARY)
mark_as_advanced(cef_dll_wrapper_INCLUDE_DIR)

add_library(cef::libcef_dll_wrapper SHARED IMPORTED)
set_property(TARGET cef::libcef_dll_wrapper PROPERTY IMPORTED_LOCATION "${cef_dll_wrapper_LIBRARY}")
set_property(TARGET cef::libcef_dll_wrapper PROPERTY IMPORTED_IMPLIB "${cef_dll_wrapper_LIBRARY}")
target_include_directories(cef::libcef_dll_wrapper INTERFACE $<INSTALL_INTERFACE:"${PROJECT_SOURCE_DIR}/deps/cef/include">)
