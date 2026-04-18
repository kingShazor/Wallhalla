#pragma once

#include <string>
#include <vector>

#include "types.h"

using std::string;
using std::vector;

namespace core_n
{
  enum fuzzy_sorter_types_t : unsigned char
  {
    MISMATCH = 0,
    FULL_MATCH = 100,
    BOUNDARY_WORD = 2,
    BOUNDARY_BOTH = BOUNDARY_WORD * 2,
    GAP_PENALTY = 5,
    MAX_GAP = 20,
    MAX_PENALTY = MAX_GAP * GAP_PENALTY,
    MATCH_CHAR = 10,
  };

  // ma score is the best :)
  i32 fzs_get_score_int( const string &text, const string &pattern );
  f32 fzs_get_score( const string &text, const string &pattern );

  vector< u32 > fzs_get_positions( const string &text, const string &pattern );
} // namespace core_n
