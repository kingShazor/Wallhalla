cmake_minimum_required(VERSION 3.27)
project(recipe_picker LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(ENABLE_PROFILING "Enable profiling-friendly build" OFF)
option(USING_ZIG "Build using zig compiler" OFF)

if(ENABLE_PROFILING)
  add_compile_options(-g -fno-omit-frame-pointer -fno-optimize-sibling-calls)
  add_link_options(-g -fno-omit-frame-pointer)
endif()

# add_compile_options(-march=native -mtune=native -O3)
# add_compile_options(-ffast-math)
add_compile_options(-fno-exceptions)
add_compile_options(-fno-rtti)
# add_compile_options(-Rpass=loop-vectorize)
# add_compile_options(-fopenmp)

# besser platformübergreifend libc++ verwenden
if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang" 
    AND NOT USING_ZIG)
    add_compile_options(-stdlib=libc++)
    add_link_options(-stdlib=libc++)
endif()


if (NOT USING_ZIG)
  include(FetchContent)
  
  FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.17.0.zip
  )
  
  FetchContent_MakeAvailable(googletest)

  target_compile_options(gtest PRIVATE -Wno-character-conversion)
target_compile_options(gtest_main PRIVATE -Wno-character-conversion)
endif()

add_library(${PROJECT_NAME} SHARED )
set_property(TARGET ${PROJECT_NAME} PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)

target_sources(${PROJECT_NAME} 
    PUBLIC
        src/simple_fuzzy_sorter.cpp
        src/recipe_picker_core.cpp
)

target_include_directories(${PROJECT_NAME} PUBLIC
  $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/src>)
if (NOT USING_ZIG)
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
          # -fsanitize=address,undefined,leak
          -Werror > )
endif()
target_compile_definitions(${PROJECT_NAME}
  PRIVATE
    $<$<PLATFORM_ID:Windows>:_CRT_NONSTDC_NO_DEPRECATE>
    $<$<PLATFORM_ID:Windows>:_CRT_SECURE_NO_DEPRECATE>
    $<$<PLATFORM_ID:Windows>:_CRT_SECURE_NO_WARNINGS>)

# set_target_properties(${PROJECT_NAME} PROPERTIES
#     WINDOWS_EXPORT_ALL_SYMBOLS ON
#     CXX_STANDARD 23
#     CXX_STANDARD_REQUIRED ON
#     CXX_EXTENSIONS OFF
#     PREFIX lib)

if (UNIX AND NOT APPLE)
  target_compile_options(${PROJECT_NAME} PRIVATE -fPIC)
endif()

install(TARGETS ${PROJECT_NAME} DESTINATION ${CMAKE_BINARY_DIR})

if (NOT USING_ZIG)

  add_executable(fuzzy_sorter_test test/fuzzy_sorter_test.cpp)
  # sanitize checks
  # if (NOT MSVC)
  #   target_link_options(${PROJECT_NAME} PRIVATE
  #     -fsanitize=address,undefined,leak )
  #   target_link_options(fuzzy_sorter_test PRIVATE
  #     -fsanitize=address,undefined,leak )
  # endif()



  # GoogleTest einbinden
  target_link_libraries(fuzzy_sorter_test
      PRIVATE
          recipe_picker    # deine Library
          gtest            # GoogleTest selbst
          gtest_main       # enthält main()
          pthread          # erforderlich für gtest
  )

  add_executable(picker_test test/picker_test.cpp)
  target_link_libraries(picker_test
      PRIVATE
          recipe_picker
          gtest
          gtest_main
          pthread
  )

  # Tests aktivieren
  enable_testing()
  add_test(NAME FuzzySorterTests COMMAND fuzzy_sorter_test)
  add_test(NAME pcikerTests COMMAND picker_test)
endif()
