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

include(FetchContent)

set(FETCHCONTENT_BASE_DIR
    "${CMAKE_BINARY_DIR}/../.deps"
)

# google-test 1.18.0
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/releases/download/v1.18.0/googletest-1.18.0.tar.gz
  URL_HASH SHA256=6e3191c1455468b3fc35a417fb565c1c5071aee1b7e7f85e30cf48a98d37d8b5
)

FetchContent_MakeAvailable(googletest)

target_compile_options(gtest PRIVATE -Wno-character-conversion)
target_compile_options(gtest_main PRIVATE -Wno-character-conversion)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

add_library(wallhallalib SHARED )

target_sources(wallhallalib 
    PUBLIC FILE_SET cxx_modules TYPE CXX_MODULES FILES
        src/types.cpp
        src/lexer/lexer.cpp
        src/util/file_reader.cpp
        src/util/file_util.cpp
)

add_executable(${PROJECT_NAME})
target_sources(${PROJECT_NAME} 
  PRIVATE
        src/main.cpp
)

target_link_libraries(${PROJECT_NAME}
      PRIVATE
        wallhallalib )

add_executable(lexer_test test/lexer_test.cpp)
target_link_libraries(lexer_test
      PRIVATE
          wallhallalib
          gtest
          gtest_main
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
