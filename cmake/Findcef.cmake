include(FindPackageHandleStandardArgs)
find_library(
    cef_LIBRARY
    NAMES cef
)
if(NOT cef_FOUND)
    set(cef_FOUND TRUE)
    set(cef_LIBRARY "${PROJECT_SOURCE_DIR}/deps/cef/Release/libcef.dll")
    set(cef_IMPLIB "${PROJECT_SOURCE_DIR}/deps/cef/Release/libcef.lib")
    set(cef_INCLUDE_DIR "${PROJECT_SOURCE_DIR}/deps/cef/include")
endif()
find_package_handle_standard_args(cef REQUIRED_VARS cef_LIBRARY cef_INCLUDE_DIR)
mark_as_advanced(cef_LIBRARY)
mark_as_advanced(cef_INCLUDE_DIR)
mark_as_advanced(cef_IMPLIB)

add_library(cef::libcef SHARED IMPORTED)
set_property(TARGET cef::libcef PROPERTY IMPORTED_LOCATION "${cef_LIBRARY}")
set_property(TARGET cef::libcef PROPERTY IMPORTED_IMPLIB "${cef_IMPLIB}")
target_include_directories(cef::libcef  INTERFACE $<INSTALL_INTERFACE:"${cef_INCLUDE_DIR}">)
