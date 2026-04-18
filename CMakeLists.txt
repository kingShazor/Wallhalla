cmake_minimum_required(VERSION 3.27)
project(wallhalla LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(ENABLE_PROFILING "Enable profiling-friendly build" OFF)
option(USING_ZIG "Build using zig compiler" OFF)

if(ENABLE_PROFILING)
  add_compile_options(-g -fno-omit-frame-pointer -fno-optimize-sibling-calls)
  add_link_options(-g -fno-omit-frame-pointer)
endif()

add_library(${PROJECT_NAME} SHARED src/ast_diff.cpp)
set_property(TARGET ${PROJECT_NAME} PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)

if (UNIX AND NOT APPLE)
  target_compile_options(${PROJECT_NAME} PRIVATE -fPIC)
endif()

install(TARGETS ${PROJECT_NAME} DESTINATION ${CMAKE_BINARY_DIR})
