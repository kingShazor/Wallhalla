cmake_minimum_required(VERSION 4.4)
project(wallhalla LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(ENABLE_PROFILING "Enable profiling-friendly build" OFF)
option(USING_ZIG "Build using zig compiler" OFF)

if(ENABLE_PROFILING)
  add_compile_options(-g -fno-omit-frame-pointer -fno-optimize-sibling-calls)
  add_link_options(-g -fno-omit-frame-pointer)
endif()


add_compile_options(-fno-exceptions)
add_compile_options(-fno-rtti)


# # besser platformübergreifend libc++ verwenden
# if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang" 
#     AND NOT USING_ZIG)
#     add_compile_options(-stdlib=libc++)
#     add_link_options(-stdlib=libc++)
# endif()
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

add_executable(${PROJECT_NAME})
target_sources(${PROJECT_NAME} 
    PUBLIC
        src/main.cpp
    PUBLIC FILE_SET cxx_modules TYPE CXX_MODULES FILES
        src/types.cpp
        src/lexer/lexer.cpp
        src/util/file_reader.cpp
        src/util/file_util.cpp
)


target_compile_options(${PROJECT_NAME}
  PRIVATE
    $<$<CXX_COMPILER_ID:MSC>:/W4>
    $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:
        -Wall
        -Wextra
        -Wpedantic
        -Wconversion
        -Wsign-conversion
        -Wshadow
        -Wunreachable-code
        -Wfloat-equal
        -Wcast-align
        -Wundef
        -Wno-shorten-64-to-32
        -Wnon-virtual-dtor
        -ftrivial-auto-var-init=pattern
        -Wno-reserved-module-identifier
        # -fsanitize=address,undefined,leak
        -Werror > )

# add_library(${PROJECT_NAME} SHARED src/ast_diff.cpp)



target_include_directories(wallhalla
    PRIVATE
        src
    PUBLIC
        include
)

set_property(TARGET ${PROJECT_NAME} PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)

if (UNIX AND NOT APPLE)
  target_compile_options(${PROJECT_NAME} PRIVATE -fPIC)
endif()

if (UNIX AND NOT APPLE)
  target_compile_options(${PROJECT_NAME} PRIVATE -fPIC)
endif()

install(TARGETS ${PROJECT_NAME} DESTINATION ${CMAKE_BINARY_DIR})
