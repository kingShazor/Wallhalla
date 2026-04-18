#include "simple_fuzzy_sorter.h"

#include <algorithm>
#include <cctype>
#include <cstring>
#include <string_view>
#include <utility>
#include <variant>

#include "types.h"

using namespace std;

using namespace core_n;

namespace
{
  enum
  {
    U_CHAR_SIZE = 256,
  };

  // small extra bonus for matching sign after oder before the pattern
  vector< bool > boundaryChars()
  {
    vector< bool > boundaries( U_CHAR_SIZE, false );
    // for ( u32 i = 0; i < U_CHAR_SIZE; ++i )
    //   boundaries[ i ] = false;

    boundaries[ '-' ] = true;
    boundaries[ '_' ] = true;
    boundaries[ ' ' ] = true;
    boundaries[ '/' ] = true;
    boundaries[ '\\' ] = true;
    boundaries[ '(' ] = true;
    boundaries[ ')' ] = true;
    boundaries[ ']' ] = true;
    boundaries[ '[' ] = true;
    boundaries[ '.' ] = true;
    boundaries[ ':' ] = true;
    boundaries[ ';' ] = true;

    return boundaries;
  }

  constexpr u32 utf8_char_length( unsigned char c )
  {
    if ( ( c & 0x80 ) == 0x00 )
      return 1; // 0xxxxxxx -> 1-byte char
    if ( ( c & 0xE0 ) == 0xC0 )
      return 2; // 110xxxxx -> 2-byte char
    if ( ( c & 0xF0 ) == 0xE0 )
      return 3; // 1110xxxx -> 3-byte char
    if ( ( c & 0xF8 ) == 0xF0 )
      return 4; // 11110xxx -> 4-byte char
    return 1; // fallback
  }

  // end is after the last found sign.
  i32 scoreBoundary( const string &text, u32 begin, u32 end )
  {
    static vector< bool > boundary{ boundaryChars() };
    i32 score{ 0 };
    if ( begin == 0 || boundary[ static_cast< unsigned char >( text[ begin - 1 ] ) ] )
      score += 2;

    if ( end == text.size() || boundary[ static_cast< unsigned char >( text[ end ] ) ] )
      score += 2;

    return score;
  }

  using result_t = variant< i32, vector< u32 > >;

  template< bool GET_POSITIONS, i32 SCORE >
  inline constexpr result_t getEmptyResult()
  {
    if constexpr ( GET_POSITIONS )
      return vector< u32 >{};
    return result_t{ SCORE };
  }

  /*
   * calcing a fast strict score (the pattern must match ascending).
   */
  template< bool GET_POSITIONS >
  result_t get_strict_score( const string &text, const string &pattern )
  {
    if ( const auto pos = text.find( pattern ); pos != std::string::npos )
    {
      const u32 patternSize{ static_cast< u32 >( pattern.size() ) };
      if constexpr ( GET_POSITIONS )
      {
        vector< u32 > positions;
        for ( u32 x{ static_cast< u32 >( pos ) }; x < pos + patternSize; ++x )
          positions.push_back( x );
        return positions;
      }

      return FULL_MATCH - BOUNDARY_BOTH + scoreBoundary( text, pos, pos + patternSize );
    }

    return getEmptyResult< GET_POSITIONS, MISMATCH >();
  }

  /*
   * fuzzy means: allowing gaps between found characters and looking also for uppercase chars
   *              we don't use UTF-8 here, because the overhead. It will only used for finding file_names
   *              so a 'ü' will have here two chars which need to match 'case sensitve'.
   *              Also meaning langugage chars count as a larger gap.
   *
   * \pattern        includes only lower case chars
   * \getPositions   true: return postions instead of score
   * \blockedIndices already blocked indices
   */
  template< bool GET_POSITIONS >
  result_t get_fuzzy_score( const string &text,
                            const string_view &pattern,
                            const string &upperPattern,
                            vector< bool > *blockedIndices = nullptr )
  {
    i32 score{ MISMATCH };
    const size_t maxStartPos{ text.size() - pattern.size() + 1 };
    // static vectors are faster
    static vector< u32 > positions;
    static vector< u32 > resultPositions;
    positions.clear();
    resultPositions.clear();

    const u32 maxScore{ static_cast< u32 >( pattern.size() * MATCH_CHAR ) };
    u32 startSearchPatternPos{ 0 };
    u32 gap{ 0 };
    u32 penalty{ 0 };

    static vector< u32 > startPosis;
    startPosis.clear();

    {
      const char patternChar{ pattern[ 0 ] };
      const char upperPatternChar{ upperPattern[ 0 ] };
      u32 i = { 0 };
      while ( ( i = text.find( patternChar, i ) ) < maxStartPos )
      {
        startPosis.push_back( i );
        ++i;
      }
      i = 0;

      while ( ( i = text.find( upperPatternChar, i ) ) < maxStartPos )
      {
        startPosis.push_back( i );
        ++i;
      }
    }

    if ( pattern.size() == 1 )
    {
      for ( const u32 startPos : startPosis )
      {
        if ( blockedIndices && ( *blockedIndices )[ startPos ] )
          continue;
        if constexpr ( GET_POSITIONS )
          return vector< u32 >{ startPos };
        return FULL_MATCH;
      }
      return getEmptyResult< GET_POSITIONS, MISMATCH >();
    }

    for ( const u32 startPos : startPosis )
    {
      if ( blockedIndices && ( *blockedIndices )[ startPos ] )
        continue;
      penalty = 0;
      startSearchPatternPos = startPos;

      // every found pattern char the max variable start pos decreases: pattern 'abcd' just need check only the first
      // three chars text 'xaxabc'
      u32 maxVarStartPos{ static_cast< u32 >( maxStartPos ) };
      for ( u32 p{ 1 }; p < pattern.size(); ++p )
      {
        const char patternChar{ pattern[ p ] };
        const char upperPatternChar{ upperPattern[ p ] };
        u32 pos{ startSearchPatternPos };
        ++maxVarStartPos;
        gap = 0;
        // find fuzzy position
        u32 maxPos = maxVarStartPos;
        if ( maxPos > pos + MAX_GAP + 1 )
          maxPos = pos + MAX_GAP + 1;
        [[ assume(maxPos <= pos + MAX_GAP + 1) ]];
        for ( ; pos < maxPos; ++pos )
        {
          // ignore blocked ranges
          if ( blockedIndices && ( *blockedIndices )[ pos ] )
            continue;
          const char textChar{ text[ pos ] };
          if ( patternChar == textChar || upperPatternChar == textChar )
            break;
        }
        if ( pos >= maxPos )
          break;

        if ( positions.empty() )
          positions.push_back( startPos );
        gap = pos - positions.back() - 1;

        penalty += ( gap * static_cast< u32 >( GAP_PENALTY ) );
        positions.push_back( pos );
        startSearchPatternPos = pos + 1;
      }

      // Impossible match, when first char can't be found
      if ( positions.size() == pattern.size() )
      {
        const i32 boundaryScore{ scoreBoundary( text, positions.front(), positions.back() + 1 ) };
        if ( penalty == 0 && boundaryScore == BOUNDARY_BOTH )
        {
          score = FULL_MATCH;
          if ( !blockedIndices )
            if constexpr ( !GET_POSITIONS )
              break;
          std::swap( positions, resultPositions );
          break;
        }

        const i32 newScore{ static_cast< i32 >( pattern.size() * MATCH_CHAR - penalty ) };
        i32 normalizedScore{
          static_cast< i32 >( static_cast< f32 >( newScore ) / static_cast< f32 >( maxScore ) * 100.0f + 0.5f ) };
        normalizedScore += ( -BOUNDARY_BOTH + boundaryScore );
        if ( normalizedScore > score )
          std::swap( positions, resultPositions );
        score = max( normalizedScore, score );
      }
      positions.clear();
    }

    if ( score != MISMATCH && blockedIndices )
      for ( const u32 i : resultPositions )
        ( *blockedIndices )[ i ] = true;

    if constexpr ( GET_POSITIONS )
      return resultPositions;
    return score;
  }

  /*
   * This Function will be called within two steps: calcing score (first step) calcing positions to highlight
   * characters (seocnd step). Telescope uses discard mode, so MISMATCHs in step one will be discarded. So when
   * positions are calculated, we know that the pattern already matches. Steps: -split pattern into tokens. tokens
   * with upper case char will be searched strictly. -calc strict or fuzzy scores -put together multi token results
   * \param getPositions true: get positions instead of a rating
   */
  template< bool GET_POSITIONS >
  result_t get_score( const string &text, const string &prompt )
  {
    if ( prompt.empty() ) // empty pattern must return match, because of discard
      return getEmptyResult< GET_POSITIONS, FULL_MATCH >();
    if ( prompt.size() == 1 ) // this will be applied on all file-names, so this must be very fast
    {
      if ( std::islower( static_cast< unsigned char >( prompt.back() ) ) ) [[ likely ]]
      {
        string promptUpper;
        promptUpper.push_back( static_cast< char >( std::toupper( static_cast< unsigned char >( prompt.back() ) ) ) );
        const auto res = get_strict_score< GET_POSITIONS >( text, promptUpper );
        if constexpr ( !GET_POSITIONS )
        {
          if ( std::get< i32 >( res ) != MISMATCH )
            return res;
        }
        else
        {
          if ( !std::get< vector< u32 > >( res ).empty() )
            return res;
        }
      }

      return get_strict_score< GET_POSITIONS >( text, prompt );
    }

    constexpr char sep{ ' ' };

    struct patternHelper_c
    {
      string pattern;
      // only set when searching in fuzzy mode for fast equal
      string upper;
      // uint utf8size;
      bool strict;
    };

    // a small cache for the last pattern - so we don't need to create every check patternHelper
    static pair< string, vector< patternHelper_c > > cachePattern;
    vector< patternHelper_c > &patternHelpers{ cachePattern.second };
    if ( cachePattern.first.size() != prompt.size() || cachePattern.first.back() != prompt.back() )
    {
      cachePattern.first = prompt;
      const string &patternString = cachePattern.first;
      patternHelpers.clear();
      bool strict = false;
      for ( u32 i{ 0 }; i < patternString.size(); ++i )
      {
        u32 y = i;
        for ( ; y < patternString.size(); ++y )
        {
          const char c{ prompt[ y ] };
          u32 byte_size{ utf8_char_length( static_cast< unsigned char >( c ) ) };
          if ( byte_size == 1 ) // ASCII
          {
            const bool isSpace{ c == sep };
            if ( isSpace )
              break;
            else if ( c > 0 && isupper( static_cast< unsigned char >( c ) ) )
              strict = true;
          }
          else
          {
            y += byte_size - 1; // y will be incremented to the next index to check via for-increment ++y
            strict = true;
          }
        }
        if ( u32 newPatternSize{ y - i }; y > 0 )
        {
          string upper;
          if ( !strict )
            for ( u32 u{ i }; u < i + newPatternSize; ++u )
              upper.push_back( static_cast< char >( toupper( static_cast< unsigned char >( patternString[ u ] ) ) ) );
          patternHelpers.push_back(
            patternHelper_c{ .pattern = patternString.substr( i, newPatternSize ), .upper = upper, .strict = strict } );
          strict = false;
          i = y;
        }
      }
    }

    if ( cachePattern.first.size() > text.size() ) [[ unlikely ]]
      return getEmptyResult< GET_POSITIONS, MISMATCH >();

    // optimization reason: reduce creation of empty vectors
    if ( patternHelpers.size() == 1 )
    {
      const auto &patternHelper = patternHelpers.back();
      return patternHelper.strict
               ? get_strict_score< GET_POSITIONS >( text, patternHelper.pattern )
               : get_fuzzy_score< GET_POSITIONS >( text, patternHelper.pattern, patternHelper.upper );
    }

    result_t result = getEmptyResult< GET_POSITIONS, MISMATCH >();
    vector< bool > blockedIndices( text.size(), false );
    for ( const auto &patternHelper : patternHelpers )
    {
      auto patternResult = patternHelper.strict ? get_strict_score< GET_POSITIONS >( text, patternHelper.pattern )
                                                : get_fuzzy_score< GET_POSITIONS >( text,
                                                                                    patternHelper.pattern,
                                                                                    patternHelper.upper,
                                                                                    &blockedIndices );
      if constexpr ( GET_POSITIONS )
      {
        auto &patternPositions = std::get< vector< u32 > >( patternResult );
        if ( patternPositions.empty() )
          return patternPositions;
        auto &positions = std::get< vector< u32 > >( result );
        if ( positions.empty() )
          std::swap( positions, patternPositions );
        else
          positions.insert( positions.end(), patternPositions.begin(), patternPositions.end() );
      }
      else
      {
        const i32 patternScore{ std::get< i32 >( patternResult ) };

        if ( patternScore == MISMATCH )
          return MISMATCH;
        std::get< i32 >( result ) += patternScore;
      }
    }

    return result;
  }
} // namespace

namespace core_n
{
  // ma score is the best :)
  i32 fzs_get_score_int( const string &text, const string &pattern )
  {
    return std::get< i32 >( get_score< false >( text, pattern ) );
  }

  f32 fzs_get_score( const string &text, const string &pattern )
  {
    const i32 score{ fzs_get_score_int( text, pattern ) };
    if ( score == MISMATCH )
      return -1.0;

    return static_cast< f32 >( 1 ) / static_cast< f32 >( score );
  }

  // positions will be displayed by the gui
  vector< u32 > fzs_get_positions( const string &text, const string &pattern )
  {
    return std::get< vector< u32 > >( get_score< true >( text, pattern ) );
  }
} // namespace core_n
