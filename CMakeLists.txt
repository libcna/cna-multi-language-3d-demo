cmake_minimum_required(VERSION 3.16)
project(cna_starfield_courier LANGUAGES CXX)

set(CMAKE_CXX_EXTENSIONS OFF)
enable_testing()

set(CNA_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../cna" CACHE PATH
    "Path to the local CNA source tree")
if(NOT EXISTS "${CNA_SOURCE_DIR}/CMakeLists.txt")
    message(FATAL_ERROR "The C++ game requires the CNA source tree at ${CNA_SOURCE_DIR}")
endif()

if(NOT DEFINED CNA_GRAPHICS_RENDERER)
    set(CNA_GRAPHICS_RENDERER "OPENGL33" CACHE STRING "CNA default renderer")
endif()
if(NOT DEFINED CNA_GRAPHICS_RENDERERS)
    set(CNA_GRAPHICS_RENDERERS "OPENGL33;OPENGLES3" CACHE STRING
        "CNA renderers compiled for the C++ reference")
endif()

set(CNA_BUILD_C_API OFF CACHE BOOL "" FORCE)
set(CNA_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(CNA_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
add_subdirectory("${CNA_SOURCE_DIR}" "${CMAKE_CURRENT_BINARY_DIR}/cna" EXCLUDE_FROM_ALL)

add_subdirectory(cpp)
