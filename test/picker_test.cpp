#include <algorithm>
#include <chrono>
#include <filesystem>
#include <gtest/gtest.h>
#include <vector>

#include "recipe_picker_core.h"
#include "types.h"

using namespace core_n;
using namespace std;

// TEST( picker, perf_test_firefox )
// {
//   // auto res = getFiles( "fd . ~/my-projects/firefox" );
//   auto res = findFileNames( "/home/shazor/my-projects/firefox", 114 );
//   EXPECT_GT( res->filteredFileNamesSize, MAX_FILES_IN_VIEW );
//   EXPECT_GT( res->fileNamesSize, 391000 );
//   deleteFileNameResult( res );
// }

// TEST( picker, perf_test_firefox_2 )
// {
//   auto res = getFiles( "fd . ~/my-projects/firefox" );
//   deleteFiles( res );
// }

// replace just mid term with '...'
TEST( picker, short_mid_algo_1_1 )
{
  const auto str = middleEllipsis(
    "testing/web-platform/tests/referrer-policy/gen/srcdoc-inherit.http-rp/unsafe-url/sharedworker-classic.http.html.headers",
    114 );
  EXPECT_LE( str.size(), 114 );
  EXPECT_EQ(
    str,
    "testing/web-platform/tests/.../gen/srcdoc-inherit.http-rp/unsafe-url/sharedworker-classic.http.html.headers" );
}

// replace mid term with '...' and remove 4 terms (nearest to mid)
TEST( picker, short_mid_algo_1_2 )
{
  const auto str = middleEllipsis(
    "testing/web-platform/tests/referrer-policy/gen/srcdoc-inherit.http-rp/unsafe-url/sharedworker-classic.http.html.headers",
    90 );
  EXPECT_LE( str.size(), 90 );
  EXPECT_EQ( str, "testing/.../unsafe-url/sharedworker-classic.http.html.headers" );
}

// replaceing the mid "buil-ins" with "..." would lead to 115 signs, so we need to remove two terms also.
TEST( picker, short_mid_algo_1_3 )
{
  const auto str = middleEllipsis(
    "js/src/tests/test262/built-ins/TypedArrayConstructors/ctors/typedarray-arg/throw-type-error-before-custom-proto-access.js",
    114 );
  EXPECT_LE( str.size(), 114 );
  EXPECT_EQ( str,
             "js/src/.../TypedArrayConstructors/ctors/typedarray-arg/throw-type-error-before-custom-proto-access.js" );
}

// file name has len: 43
TEST( picker, root_short_mid_algo_1_5_dirs_replace_just_mid )
{
  const auto str = middleEllipsis( "/aaaaa/bbbbb/ccccc/ddddd/eeeee/longFile.txt", 41 );
  EXPECT_EQ( str.size(), 41 );
  EXPECT_EQ( str, "/aaaaa/.../ccccc/ddddd/eeeee/longFile.txt" );
}

// file has name len: 42
TEST( picker, short_mid_algo_1_5_dirs_replace_just_mid )
{
  const auto str = middleEllipsis( "aaaaa/bbbbb/ccccc/ddddd/eeeee/longFile.txt", 40 );
  EXPECT_EQ( str.size(), 40 );
  EXPECT_EQ( str, "aaaaa/bbbbb/.../ddddd/eeeee/longFile.txt" );
}

// file name has len: 42
TEST( picker, short_mid_algo_1_5_dirs_replace_3_terms )
{
  const auto str = middleEllipsis( "aaaaa/bbbbb/ccccc/ddddd/eeeee/longFile.txt", 39 );
  EXPECT_EQ( str.size(), 28 );
  EXPECT_EQ( str, "aaaaa/.../eeeee/longFile.txt" );
}

// file name has len: 42
TEST( picker, short_mid_algo_1_5_dirs_replace_5_terms )
{
  const auto str = middleEllipsis( "aaaaa/bbbbb/ccccc/ddddd/eeeee/longFile.txt", 27 );
  EXPECT_EQ( str.size(), 16 );
  EXPECT_EQ( str, ".../longFile.txt" );
}

// file name has len: 42
TEST( picker, short_mid_algo_1_5_dirs_fallback_to_algo_2 )
{
  const auto str = middleEllipsis( "aaaaa/bbbbb/ccccc/ddddd/eeeee/longFile.txt", 8 );
  EXPECT_EQ( str.size(), 8 );
  EXPECT_EQ( str, "..le.txt" );
}

// file name has len: 37
TEST( picker, root_short_mid_algo_1_4_dirs_replace_just_mid )
{
  const auto str = middleEllipsis( "/aaaaa/bbbbb/ccccc/ddddd/longFile.txt", 35 );
  EXPECT_EQ( str.size(), 35 );
  EXPECT_EQ( str, "/aaaaa/.../ccccc/ddddd/longFile.txt" );
}

// file name has len: 36
TEST( picker, short_mid_algo_1_4_dirs_replace_just_mid )
{
  const auto str = middleEllipsis( "aaaaa/bbbbb/ccccc/ddddd/longFile.txt", 35 );
  EXPECT_EQ( str.size(), 34 );
  EXPECT_EQ( str, "aaaaa/.../ccccc/ddddd/longFile.txt" );
}

// file name has len: 36
TEST( picker, short_mid_algo_1_4_dirs_remove_3_dirs )
{
  const auto str = middleEllipsis( "aaaaa/bbbbb/ccccc/ddddd/longFile.txt", 23 );
  EXPECT_EQ( str.size(), 22 );
  EXPECT_EQ( str, ".../ddddd/longFile.txt" );
}

// file name has len: 36
TEST( picker, short_mid_algo_1_4_dirs_remove_4_dirs )
{
  const auto str = middleEllipsis( "aaaaa/bbbbb/ccccc/ddddd/longFile.txt", 17 );
  EXPECT_EQ( str.size(), 16 );
  EXPECT_EQ( str, ".../longFile.txt" );
}

TEST( picker, short_mid_algo_2_1 )
{
  const auto str = middleEllipsis(
    "testing/web-platform/tests/referrer-policy/gen/srcdoc-inherit.http-rp/unsafe-url/sharedworker-classic.http.html.headers",
    30 );
  EXPECT_EQ( str.size(), 30 );
  EXPECT_EQ( str, "..er-classic.http.html.headers" );
}

TEST( picker, short_mid_algo_2_2 )
{
  const auto str = middleEllipsis( "t/x/y/longFileName.txt", 10 );
  EXPECT_EQ( str.size(), 10 );
  EXPECT_EQ( str, "..Name.txt" );
}

TEST( picker, short_mid_algo_2_3 )
{
  const auto str = middleEllipsis( "verylongFileName.txt", 10 );
  EXPECT_EQ( str.size(), 10 );
  EXPECT_EQ( str, "..Name.txt" );
}

// Index of Name
TEST( picker, short_mid_algo_2_pos )
{
  vector< u32 > pos = { 14, 15, 16, 17 };
  const auto str = middleEllipsis( "t/x/y/longFileName.txt", 10, &pos );
  EXPECT_EQ( str.size(), 10 );
  EXPECT_EQ( str, "..Name.txt" );
  EXPECT_EQ( pos.size(), 4 );
  EXPECT_EQ( pos[ 0 ], 2 );
  EXPECT_EQ( pos[ 1 ], 3 );
  EXPECT_EQ( pos[ 2 ], 4 );
  EXPECT_EQ( pos[ 3 ], 5 );
}

// Index of .txt
TEST( picker, short_mid_algo_2_pos_2 )
{
  vector< u32 > pos = { 18, 19, 20, 21 };
  const auto str = middleEllipsis( "t/x/y/longFileName.txt", 10, &pos );
  EXPECT_EQ( str.size(), 10 );
  EXPECT_EQ( str, "..Name.txt" );
  EXPECT_EQ( pos.size(), 4 );
  EXPECT_EQ( pos[ 0 ], 6 );
  EXPECT_EQ( pos[ 1 ], 7 );
  EXPECT_EQ( pos[ 2 ], 8 );
  EXPECT_EQ( pos[ 3 ], 9 );
}

// Index of eNam
TEST( picker, short_mid_algo_2_pos_3 )
{
  vector< u32 > pos = { 13, 14, 15, 16 };
  const auto str = middleEllipsis( "t/x/y/longFileName.txt", 10, &pos );
  EXPECT_EQ( str.size(), 10 );
  EXPECT_EQ( str, "..Name.txt" );
  EXPECT_EQ( pos.size(), 5 );
  EXPECT_EQ( pos[ 0 ], 0 );
  EXPECT_EQ( pos[ 1 ], 1 );
  EXPECT_EQ( pos[ 2 ], 2 );
  EXPECT_EQ( pos[ 3 ], 3 );
  EXPECT_EQ( pos[ 4 ], 4 );
}

// Index of leNa
TEST( picker, short_mid_algo_2_pos_4 )
{
  vector< u32 > pos = { 12, 13, 14, 15 };
  const auto str = middleEllipsis( "t/x/y/longFileName.txt", 10, &pos );
  EXPECT_EQ( str.size(), 10 );
  EXPECT_EQ( str, "..Name.txt" );
  EXPECT_EQ( pos.size(), 4 );
  EXPECT_EQ( pos[ 0 ], 0 );
  EXPECT_EQ( pos[ 1 ], 1 );
  EXPECT_EQ( pos[ 2 ], 2 );
  EXPECT_EQ( pos[ 3 ], 3 );
}

// file has name len: 42
TEST( picker, short_mid_algo_1_5_dirs_replace_1_test_pos_1 )
{
  // File
  vector< u32 > pos = { 34, 35, 36, 37 };
  const auto str = middleEllipsis( "aaaaa/bbbbb/ccccc/ddddd/eeeee/longFile.txt", 40, &pos );
  EXPECT_EQ( str.size(), 40 );
  EXPECT_EQ( str, "aaaaa/bbbbb/.../ddddd/eeeee/longFile.txt" );
  EXPECT_EQ( pos.size(), 4 );
  EXPECT_EQ( pos[ 0 ], 32 );
  EXPECT_EQ( pos[ 1 ], 33 );
  EXPECT_EQ( pos[ 2 ], 34 );
  EXPECT_EQ( pos[ 3 ], 35 );
}

// file has name len: 42
TEST( picker, short_mid_algo_1_5_dirs_replace_1_test_pos_2 )
{
  // .txt
  vector< u32 > pos = { 38, 39, 40, 41 };
  const auto str = middleEllipsis( "aaaaa/bbbbb/ccccc/ddddd/eeeee/longFile.txt", 40, &pos );
  EXPECT_EQ( str.size(), 40 );
  EXPECT_EQ( str, "aaaaa/bbbbb/.../ddddd/eeeee/longFile.txt" );
  EXPECT_EQ( pos.size(), 4 );
  EXPECT_EQ( pos[ 0 ], 36 );
  EXPECT_EQ( pos[ 1 ], 37 );
  EXPECT_EQ( pos[ 2 ], 38 );
  EXPECT_EQ( pos[ 3 ], 39 );
}

// file has name len: 42
TEST( picker, short_mid_algo_1_5_dirs_replace_1_test_pos_3 )
{
  // dddd
  vector< u32 > pos = { 18, 19, 20, 21 };
  const auto str = middleEllipsis( "aaaaa/bbbbb/ccccc/ddddd/eeeee/longFile.txt", 40, &pos );
  EXPECT_EQ( str.size(), 40 );
  EXPECT_EQ( str, "aaaaa/bbbbb/.../ddddd/eeeee/longFile.txt" );
  EXPECT_EQ( pos.size(), 4 );
  EXPECT_EQ( pos[ 0 ], 16 );
  EXPECT_EQ( pos[ 1 ], 17 );
  EXPECT_EQ( pos[ 2 ], 18 );
  EXPECT_EQ( pos[ 3 ], 19 );
}

// file has name len: 42
TEST( picker, short_mid_algo_1_5_dirs_replace_1_test_pos_4 )
{
  // /ddd
  vector< u32 > pos = { 17, 18, 19, 20 };
  const auto str = middleEllipsis( "aaaaa/bbbbb/ccccc/ddddd/eeeee/longFile.txt", 40, &pos );
  EXPECT_EQ( str.size(), 40 );
  EXPECT_EQ( str, "aaaaa/bbbbb/.../ddddd/eeeee/longFile.txt" );
  EXPECT_EQ( pos.size(), 6 );
  EXPECT_EQ( pos[ 0 ], 12 );
  EXPECT_EQ( pos[ 1 ], 13 );
  EXPECT_EQ( pos[ 2 ], 14 );
  EXPECT_EQ( pos[ 3 ], 16 );
  EXPECT_EQ( pos[ 4 ], 17 );
  EXPECT_EQ( pos[ 5 ], 18 );
}

// file has name len: 42
TEST( picker, short_mid_algo_1_5_dirs_replace_1_test_pos_5 )
{
  // c/dd
  vector< u32 > pos = { 16, 17, 18, 19 };
  const auto str = middleEllipsis( "aaaaa/bbbbb/ccccc/ddddd/eeeee/longFile.txt", 40, &pos );
  EXPECT_EQ( str.size(), 40 );
  EXPECT_EQ( str, "aaaaa/bbbbb/.../ddddd/eeeee/longFile.txt" );
  EXPECT_EQ( pos.size(), 5 );
  EXPECT_EQ( pos[ 0 ], 12 );
  EXPECT_EQ( pos[ 1 ], 13 );
  EXPECT_EQ( pos[ 2 ], 14 );
  EXPECT_EQ( pos[ 3 ], 16 );
  EXPECT_EQ( pos[ 4 ], 17 );
}

// file has name len: 42
TEST( picker, short_mid_algo_1_5_dirs_replace_1_test_pos_6 )
{
  // cc/d
  vector< u32 > pos = { 15, 16, 17, 18 };
  const auto str = middleEllipsis( "aaaaa/bbbbb/ccccc/ddddd/eeeee/longFile.txt", 40, &pos );
  EXPECT_EQ( str.size(), 40 );
  EXPECT_EQ( str, "aaaaa/bbbbb/.../ddddd/eeeee/longFile.txt" );
  EXPECT_EQ( pos.size(), 4 );
  EXPECT_EQ( pos[ 0 ], 12 );
  EXPECT_EQ( pos[ 1 ], 13 );
  EXPECT_EQ( pos[ 2 ], 14 );
  EXPECT_EQ( pos[ 3 ], 16 );
}

// file has name len: 42
TEST( picker, short_mid_algo_1_5_dirs_replace_1_test_pos_7 )
{
  // ccc/
  vector< u32 > pos = { 14, 15, 16, 17 };
  const auto str = middleEllipsis( "aaaaa/bbbbb/ccccc/ddddd/eeeee/longFile.txt", 40, &pos );
  EXPECT_EQ( str.size(), 40 );
  EXPECT_EQ( str, "aaaaa/bbbbb/.../ddddd/eeeee/longFile.txt" );
  EXPECT_EQ( pos.size(), 3 );
  EXPECT_EQ( pos[ 0 ], 12 );
  EXPECT_EQ( pos[ 1 ], 13 );
  EXPECT_EQ( pos[ 2 ], 14 );
}

// file has name len: 42
TEST( picker, short_mid_algo_1_5_dirs_replace_1_test_pos_8 )
{
  // cccc
  vector< u32 > pos = { 13, 14, 15, 16 };
  const auto str = middleEllipsis( "aaaaa/bbbbb/ccccc/ddddd/eeeee/longFile.txt", 40, &pos );
  EXPECT_EQ( str.size(), 40 );
  EXPECT_EQ( str, "aaaaa/bbbbb/.../ddddd/eeeee/longFile.txt" );
  EXPECT_EQ( pos.size(), 3 );
  EXPECT_EQ( pos[ 0 ], 12 );
  EXPECT_EQ( pos[ 1 ], 13 );
  EXPECT_EQ( pos[ 2 ], 14 );
}

// file has name len: 42
TEST( picker, short_mid_algo_1_5_dirs_replace_1_test_pos_9 )
{
  // cccc
  vector< u32 > pos = { 12, 13, 14, 15 };
  const auto str = middleEllipsis( "aaaaa/bbbbb/ccccc/ddddd/eeeee/longFile.txt", 40, &pos );
  EXPECT_EQ( str.size(), 40 );
  EXPECT_EQ( str, "aaaaa/bbbbb/.../ddddd/eeeee/longFile.txt" );
  EXPECT_EQ( pos.size(), 3 );
  EXPECT_EQ( pos[ 0 ], 12 );
  EXPECT_EQ( pos[ 1 ], 13 );
  EXPECT_EQ( pos[ 2 ], 14 );
}

// file has name len: 42
TEST( picker, short_mid_algo_1_5_dirs_replace_1_test_pos_10 )
{
  // /ccc
  vector< u32 > pos = { 11, 12, 13, 14 };
  const auto str = middleEllipsis( "aaaaa/bbbbb/ccccc/ddddd/eeeee/longFile.txt", 40, &pos );
  EXPECT_EQ( str.size(), 40 );
  EXPECT_EQ( str, "aaaaa/bbbbb/.../ddddd/eeeee/longFile.txt" );
  EXPECT_EQ( pos.size(), 4 );
  EXPECT_EQ( pos[ 0 ], 11 );
  EXPECT_EQ( pos[ 1 ], 12 );
  EXPECT_EQ( pos[ 2 ], 13 );
  EXPECT_EQ( pos[ 3 ], 14 );
}

// file has name len: 42
TEST( picker, short_mid_algo_1_5_dirs_replace_1_test_pos_11 )
{
  // b/cc
  vector< u32 > pos = { 10, 11, 12, 13 };
  const auto str = middleEllipsis( "aaaaa/bbbbb/ccccc/ddddd/eeeee/longFile.txt", 40, &pos );
  EXPECT_EQ( str.size(), 40 );
  EXPECT_EQ( str, "aaaaa/bbbbb/.../ddddd/eeeee/longFile.txt" );
  EXPECT_EQ( pos.size(), 5 );
  EXPECT_EQ( pos[ 0 ], 10 );
  EXPECT_EQ( pos[ 1 ], 11 );
  EXPECT_EQ( pos[ 2 ], 12 );
  EXPECT_EQ( pos[ 3 ], 13 );
  EXPECT_EQ( pos[ 4 ], 14 );
}

// file has name len: 42
TEST( picker, short_mid_algo_1_5_dirs_replace_1_test_pos_12 )
{
  // bb/c
  vector< u32 > pos = { 9, 10, 11, 12 };
  const auto str = middleEllipsis( "aaaaa/bbbbb/ccccc/ddddd/eeeee/longFile.txt", 40, &pos );
  EXPECT_EQ( str.size(), 40 );
  EXPECT_EQ( str, "aaaaa/bbbbb/.../ddddd/eeeee/longFile.txt" );
  EXPECT_EQ( pos.size(), 6 );
  EXPECT_EQ( pos[ 0 ], 9 );
  EXPECT_EQ( pos[ 1 ], 10 );
  EXPECT_EQ( pos[ 2 ], 11 );
  EXPECT_EQ( pos[ 3 ], 12 );
  EXPECT_EQ( pos[ 4 ], 13 );
  EXPECT_EQ( pos[ 5 ], 14 );
}

// file has name len: 42
TEST( picker, short_mid_algo_1_5_dirs_replace_1_test_pos_13 )
{
  // bbb/
  vector< u32 > pos = { 8, 9, 10, 11 };
  const auto str = middleEllipsis( "aaaaa/bbbbb/ccccc/ddddd/eeeee/longFile.txt", 40, &pos );
  EXPECT_EQ( str.size(), 40 );
  EXPECT_EQ( str, "aaaaa/bbbbb/.../ddddd/eeeee/longFile.txt" );
  EXPECT_EQ( pos.size(), 4 );
  EXPECT_EQ( pos[ 0 ], 8 );
  EXPECT_EQ( pos[ 1 ], 9 );
  EXPECT_EQ( pos[ 2 ], 10 );
  EXPECT_EQ( pos[ 3 ], 11 );
}

namespace
{
  void search( const char *prompt )
  {
    using namespace std::chrono;
    auto start = high_resolution_clock::now();
    auto res = filterFileNames( prompt );
    auto end = high_resolution_clock::now();
    auto duration = duration_cast< milliseconds >( end - start );
    // println( "param {} duration in ms: {}", prompt, duration.count() );
    deleteFileNameResult( res );
  }
} // namespace

TEST( picker, perf_test_firefox )
{
  const string home = std::getenv( "HOME" );
  const auto path = std::filesystem::path( home ) / "my-projects/firefox";
  auto res = findFileNames( path.c_str(), nullptr, 114 );
  EXPECT_GT( res->filteredFileNamesSize, MAX_FILES_IN_VIEW );
  EXPECT_GT( res->fileNamesSize, 391000 );
  deleteFileNameResult( res );

  search( "u" );
  search( "un" );
  search( "uns" );
  search( "unsw" );
  search( "unswr" );
  search( "unswap" );
  search( "unswapp" );
  search( "unswappe" );
  search( "unsawappe" );
  res = filterFileNames( "unsafwrappe" );
  EXPECT_EQ( 1, res->filteredFileNamesSize );
  if ( res->fileNamesSize == 1 )
    EXPECT_TRUE( string( res->viewFileNames[ 0 ] ).ends_with( "unsafe_wrapper.rs" ) );
  deleteFileNameResult( res );
}

TEST( pickerPerf, perf_test_firefox )
{
  const string home = std::getenv( "HOME" );
  const auto path = std::filesystem::path( home ) / "my-projects/firefox";
  auto res = findFileNames( path.c_str(), nullptr, 114 );
  EXPECT_GT( res->filteredFileNamesSize, MAX_FILES_IN_VIEW );
  EXPECT_GT( res->fileNamesSize, 391000 );
  deleteFileNameResult( res );
}

TEST( pickerPerf, perf_test_grep )
{
  const auto path = std::filesystem::current_path();
  auto res = grepWord( "rg -o '{}' --column 2>/dev/null | head -n 20000", path.parent_path().c_str(), "vector<", 100 );
  // println( "res viewFileNameSize: {}", res->viewFileNamesSize );
  EXPECT_GT( res->viewFileNamesSize, 50 );
  for ( u32 i = 0; i < res->viewFileNamesSize; ++i )
  {
    // std::println( "viewFileNames: {}", res->viewFileNames[ i ] );
    const auto res = getFullFileName( i );

    // std::println("str: {}",res);
    EXPECT_TRUE(res);
    // if (res)
  }
  deleteFileNameResult( res );
}

TEST( picker, perf_test_find_cpp )
{
  const auto path = std::filesystem::current_path().parent_path();
  auto res = findFileNames( path.c_str(), "cpp", 114 );
  EXPECT_EQ( res->fileNamesSize, 4 );
  vector< string > expectedFiles{ "src/recipe_picker_core.cpp",
                                  "src/simple_fuzzy_sorter.cpp",
                                  "test/fuzzy_sorter_test.cpp",
                                  "test/picker_test.cpp" };
  vector< string > foundFiles;
  for ( u32 i = 0; i < res->fileNamesSize; ++i )
    foundFiles.push_back( res->viewFileNames[ i ] );

  std::ranges::sort( foundFiles );
  for ( u32 i = 0; i < res->fileNamesSize; ++i )
    EXPECT_EQ( foundFiles[ i ], expectedFiles[ i ] );
}

TEST( picker, perf_test_find_cpp_h )
{
  const auto path = std::filesystem::current_path().parent_path();
  auto res = findFileNames( path.c_str(), "cpp,h", 114 );
  EXPECT_EQ( res->fileNamesSize, 7 );
  vector< string > expectedFiles{ "src/recipe_picker_core.cpp",
                                  "src/recipe_picker_core.h",
                                  "src/simple_fuzzy_sorter.cpp",
                                  "src/simple_fuzzy_sorter.h",
                                  "src/types.h",
                                  "test/fuzzy_sorter_test.cpp",
                                  "test/picker_test.cpp" };
  vector< string > foundFiles;
  for ( u32 i = 0; i < res->fileNamesSize; ++i )
    foundFiles.push_back( res->viewFileNames[ i ] );

  std::ranges::sort( foundFiles );
  for ( u32 i = 0; i < res->fileNamesSize; ++i )
    EXPECT_EQ( foundFiles[ i ], expectedFiles[ i ] );
}

//
//   using namespace std::chrono;
//   auto start = high_resolution_clock::now();
//   vector< result_t * > results;
//
//   for ( auto i = 1u; i <= 50; ++i )
//   {
//     results.push_back( filterFileNames( "u" ) );
//     results.push_back( filterFileNames( "un" ) );
//     results.push_back( filterFileNames( "" ) );
//     println( "{}% completed", i*2);
//   }
//
//   auto end = high_resolution_clock::now();
//   auto duration = duration_cast< milliseconds >( end - start );
//   println( "50 searches duration in ms: {}", duration.count() );
//   for ( auto resEntry : results )
//     deleteFileNameResult( resEntry );
// }

// TEST( pickerPerf, perf_test_firefox )
// {
//   constexpr std::size_t N = 10000000;
//   std::vector< float > a( N, 1.0f );
//   std::vector< float > b( N, 2.0f );
// #pragma omp simd
//   for ( size_t i = 0; i < N; ++i )
//   {
//     a[ i ] += b[ i ];
//   }
//   EXPECT_EQ( a[0], 3.0f );
// }
