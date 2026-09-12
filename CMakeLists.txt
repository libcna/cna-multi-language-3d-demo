cmake_minimum_required(VERSION 3.16)
project(cna_starfield_courier LANGUAGES C CXX)

set(CMAKE_CXX_EXTENSIONS OFF)
enable_testing()

option(STARFIELD_ENABLE_CNA "Build the demo's consumer check against ../cna" OFF)
set(CNA_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../cna" CACHE PATH
    "Path to the local CNA source tree")

if(STARFIELD_ENABLE_CNA)
    if(NOT EXISTS "${CNA_SOURCE_DIR}/CMakeLists.txt")
        message(FATAL_ERROR "STARFIELD_ENABLE_CNA requires a CNA source tree at ${CNA_SOURCE_DIR}")
    endif()
    set(CNA_BUILD_C_API ON CACHE BOOL "" FORCE)
    set(CNA_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(CNA_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    add_subdirectory("${CNA_SOURCE_DIR}" "${CMAKE_CURRENT_BINARY_DIR}/cna" EXCLUDE_FROM_ALL)
endif()

add_subdirectory(cpp)
add_subdirectory(c)