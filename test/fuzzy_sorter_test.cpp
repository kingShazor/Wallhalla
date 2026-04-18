#include <chrono>
#include <fstream>
#include <gtest/gtest.h>
#include <iostream>
#include <string>

#include "simple_fuzzy_sorter.h"

using namespace core_n;

// ----------- fuzzy-match-file-name-tests -------

TEST( FuzzySorter, empty_pattern_test )
{
  auto score = fzs_get_score_int( "init.lua", "" );
  EXPECT_EQ( score, FULL_MATCH );
}

TEST( FuzzySorter, fuzzy_file_match )
{
  auto score = fzs_get_score_int( "init.lua", "init" );
  EXPECT_EQ( score, FULL_MATCH );
}

TEST( FuzzySorter, fuzzy_file_match_2 )
{
  auto score = fzs_get_score_int( "/home/shazor/.config/nvim/init.lua", "init" );
  EXPECT_EQ( score, FULL_MATCH );
}

// das nächste zeichen ist kein Wortanfang (erwartet $ oder '.', '('...
TEST( FuzzySorter, fuzzy_file_match_missing_end_bonus )
{
  auto score = fzs_get_score_int( "init.lua", "init." );
  EXPECT_EQ( score, FULL_MATCH - BOUNDARY_WORD );
}

// das erste zeichen ist kein Wortanfang
TEST( FuzzySorter, fuzzy_file_match_missing_start_bonus )
{
  auto score = fzs_get_score_int( "init.lua", "nit" );
  EXPECT_EQ( score, FULL_MATCH - BOUNDARY_WORD );
}

// missing both word boundarie missing both word boundaries
TEST( FuzzySorter, fuzzy_file_match_missing_both_bounds )
{
  auto score = fzs_get_score_int( "init.lua", "ni" );
  EXPECT_EQ( score, FULL_MATCH - BOUNDARY_BOTH );
}

TEST( FuzzySorter, fuzzy_file_mapping_suggest )
{
  auto score = fzs_get_score_int( "mapping_suggest_station_header.cpp", "mapping suggest st" );
  EXPECT_EQ( score, FULL_MATCH * 3 - BOUNDARY_WORD );
}

TEST( FuzzySorter, fuzzy_file_location_util )
{
  auto score = fzs_get_score_int( "integration_location_util.cpp", "location util" );
  EXPECT_EQ( score, FULL_MATCH * 2 );
}

TEST( FuzzySorter, fuzzy_file_util_location )
{
  auto score = fzs_get_score_int( "integration_location_util.cpp", "util location" );
  EXPECT_EQ( score, FULL_MATCH * 2 );
}

TEST( FuzzySorter, fuzzy_file_in_lo_ut )
{
  auto score = fzs_get_score_int( "integration_location_util.cpp", "in lo ut" );
  EXPECT_EQ( score, FULL_MATCH * 3 - BOUNDARY_WORD * 3 );
}

TEST( FuzzySorter, fuzzy_file_very_fuzzy )
{
  auto score = fzs_get_score_int( "mapping_suggest_station_header.cpp", "mpns" );
  EXPECT_GT( score, 30 );
  EXPECT_LT( score, 40 );
}

TEST( FuzzySorter, fuzzy_file_upper_case_also_match )
{
  auto score = fzs_get_score_int( "INTEGRATION.cmake", "int cmake" );
  EXPECT_EQ( score, FULL_MATCH * 2 - BOUNDARY_WORD );
}

TEST( FuzzySorter, fuzzy_file_very_important_char_1 )
{
  auto score = fzs_get_score_int( "übertrieben.xml", "über" );
  EXPECT_EQ( score, FULL_MATCH - BOUNDARY_WORD );
}

// über will be a sctrict search
TEST( FuzzySorter, fuzzy_file_very_important_char_2 )
{
  auto score = fzs_get_score_int( "üBertrieben.xml", "über" );
  EXPECT_EQ( score, MISMATCH );
}

TEST( FuzzySorter, fuzzy_file_very_important_char_3 )
{
  auto score = fzs_get_score_int( "übertriebenerText.xml", "über text" );
  EXPECT_EQ( score, FULL_MATCH * 2 - 2 * BOUNDARY_WORD );
}

// Not - supported Utf8-fuzzy search
TEST( FuzzySorter, fuzzy_file_very_important_char_4 )
{
  auto score = fzs_get_score_int( "Übertrieben.xml", "über" );
  EXPECT_EQ( score, MISMATCH );
}

TEST( FuzzySorter, NoMatch )
{
  auto score = fzs_get_score_int( "init.lua", "vim" );
  EXPECT_EQ( score, MISMATCH );
}

const char *poem = "Twinkle, twinkle, little star, "
                   "How I wonder what you are! "
                   "Up above the world so high, "
                   "Like a diamond in the sky.";

TEST( FuzzySorter, fuzzy_poem_match )
{
  auto score = fzs_get_score_int( poem, "above" );
  EXPECT_EQ( score, FULL_MATCH );
}

TEST( FuzzySorter, fuzzy_poem_mismatch )
{
  auto score = fzs_get_score_int( poem, "alien" );
  EXPECT_EQ( score, MISMATCH );
}

TEST( FuzzySorter, fuzzy_poem_wrd )
{
  auto score = fzs_get_score_int( poem, "wrd" );
  EXPECT_EQ( score, 67 ); // 3 matches + 2 gaps => 20/30
}

// found text phrase are 2 words: "rld s"
TEST( FuzzySorter, fuzzy_poem_rds )
{
  auto score = fzs_get_score_int( poem, "rds" );
  EXPECT_EQ( score, 67 - BOUNDARY_BOTH ); // 3 matches + 2 gaps => 20/30
}

TEST( FuzzySorter, fuzzy_file_pos )
{
  auto posis = fzs_get_positions( "init.lua", "init" );
  EXPECT_EQ( posis.size(), 4 );
  EXPECT_EQ( posis[ 0 ], 0 );
  EXPECT_EQ( posis[ 1 ], 1 );
  EXPECT_EQ( posis[ 2 ], 2 );
  EXPECT_EQ( posis[ 3 ], 3 );
}

TEST( FuzzySorter, fuzzy_file_pos_missing_both_bounds )
{
  auto posis = fzs_get_positions( "init.lua", "ni" );
  EXPECT_EQ( posis.size(), 2 );
  EXPECT_EQ( posis[ 0 ], 1 );
  EXPECT_EQ( posis[ 1 ], 2 );
}

TEST( FuzzySorter, fuzzy_file_upper_case_pos )
{
  auto posis = fzs_get_positions( "INTEGRATION.cmake", "INT cmake" );
  EXPECT_EQ( posis.size(), 8 );
  EXPECT_EQ( posis[ 0 ], 0 );
  EXPECT_EQ( posis[ 1 ], 1 );
  EXPECT_EQ( posis[ 2 ], 2 );
  EXPECT_EQ( posis[ 3 ], 12 );
  EXPECT_EQ( posis[ 4 ], 13 );
  EXPECT_EQ( posis[ 5 ], 14 );
  EXPECT_EQ( posis[ 6 ], 15 );
}

TEST( FuzzySorter, fuzzy_file_upper_case_only )
{
  auto score = fzs_get_score_int( "README.md", "read" );
  EXPECT_EQ( score, FULL_MATCH - BOUNDARY_WORD );
}

TEST( FuzzySorter, fuzzy_word_mismatch_leads_to_distract )
{
  auto score = fzs_get_score_int( "mapping_suggest_station_header.cpp", "map sug ope" );
  EXPECT_EQ( score, MISMATCH );
}

TEST( FuzzySorter, fuzzy_do_not_use_same_word )
{
  auto score = fzs_get_score_int( "tmpl/unique_type_range.h", "que ue" );
  EXPECT_EQ( score, MISMATCH );
}

TEST( FuzzySorter, fuzzy_do_not_use_same_word_but_find_word )
{
  auto score = fzs_get_score_int( "network/mail_queue.cpp", "que ue" );
  EXPECT_EQ( score, FULL_MATCH * 2 - 2 * BOUNDARY_WORD );
}

TEST( FuzzySorter, fuzzy_seg_fault )
{
  auto score = fzs_get_score_int( "mach", "wrapper" );
  EXPECT_EQ( score, MISMATCH );
}

TEST( FuzzySorter, fuzzy_match_long_gap )
{
  auto score = fzs_get_score_int( "modules/integration/src/integration/mapping/mapping_suggest_station_header.cpp", "mappingstat" );
  EXPECT_GT( score, MATCH_CHAR );
}

TEST( FuzzySorter, fuzzy_serch_for_one_char )
{
  using namespace std;
  using namespace std::chrono;
  auto start = high_resolution_clock::now();
  auto score = fzs_get_score_int( "network/Mail_queue.cpp", "m" );
  EXPECT_EQ( score, FULL_MATCH - BOUNDARY_WORD );
  auto end = high_resolution_clock::now();
  auto duration = duration_cast< milliseconds >( end - start );
  cout << "duration in ms:" << duration.count() << endl;
}

// TEST( FuzzySorter, fuzzy_perf_test )
// {
//   std::vector< std::string > src_files = { "include/logging/logger.h",
//                                            "include/logging/logger.cpp",
//                                            "include/cfg/config.h",
//                                            "src/import/config_parser.cpp",
//                                            "include/network/network_manager.h",
//                                            "src/network/network_manager.cpp",
//                                            "include/core/file_utils.h",
//                                            "src/core/file_utils.cpp",
//                                            "include/engine/math_engine.h",
//                                            "src/engine/math_engine.cpp",
//                                            "include/engine/renderer.h",
//                                            "src/engine/renderer.cpp",
//                                            "include/event/event_system.h",
//                                            "src/event/event_system.cpp",
//                                            "include/util/string_helpers.h",
//                                            "src/database/database_connector.cpp",
//                                            "src/core/resource_loader.cpp",
//                                            "include/core/thread_pool.h",
//                                            "src/core/memory_manager.cpp",
//                                            "include/logging/error_handler.h" };
//   const uint n = 6000000;
//   const uint rounds = n / src_files.size();
//   const char *searchWord = "f"; //"util engine cpp";
//   // const char *searchWord = "math engine cpp";
//   for ( int i = 0; i < rounds; ++i )
//     for ( const auto &file : src_files )
//     {
//       const auto score = fzs_get_score( file.data(), searchWord );
//       if ( score != MISMATCH )
//         fzs_get_positions( file.data(), searchWord );
//     }
// }

TEST( FuzzySorter, fuzzy_perf_firefox )
{
  using namespace std;
  using namespace std::chrono;
  vector< string > filenames;
  std::string file = "/home/shazor/firefox_files.txt";
  ifstream input( file, std::ios::in );
  if ( !input.is_open() )
  {
    cout << "not open :-(";
    return;
  }

  string filename;
  while ( std::getline( input, filename ) )
  {
    filenames.push_back( filename );
  }

  cout << "filenames read: " << filenames.size() << endl;

  // const char *pattern = "wrapper unsafe";
  const char *pattern = "un";

  auto start = high_resolution_clock::now();
  for ( int i = 0; i < 100; ++i )
  {
    cout << "done: " << i << "%" << endl;
    for ( const auto &text : filenames )
      auto score = fzs_get_score_int( text.c_str(), pattern );
  }

  auto end = high_resolution_clock::now();
  auto duration = duration_cast< milliseconds >( end - start );
  cout << "duration in ms:" << duration.count() << endl;
}
