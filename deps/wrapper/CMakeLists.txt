cmake_minimum_required(VERSION 3.30)

project(extendify_wrapper)

if (WIN32)
    list(APPEND srcs src/win32.c)
    list(APPEND CMAKE_C_FLAGS "-Xlinker /export:DetourFinishHelperProcess,@1,NONAME")
elseif (LINUX)
    list(APPEND srcs src/linux.c)
else ()
    message(FATAL_ERROR "OS not supported")
endif ()

list(APPEND CMAKE_C_FLAGS "-gdwarf -O0")

add_library(extendify_wrapper SHARED ${srcs})


install(PROGRAMS $<TARGET_FILE:extendify_wrapper> TYPE LIB)
