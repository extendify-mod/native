cmake_minimum_required(VERSION 3.30)

project(extendify_wrapper)

if (WIN32)
    list(APPEND srcs src/win32.c)
elseif (LINUX)
    list(APPEND srcs src/linux.c)
else ()
    message(FATAL_ERROR "OS not supported")
endif ()

add_library(extendify_wrapper SHARED ${srcs})
