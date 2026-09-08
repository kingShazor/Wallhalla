#include <gtest/gtest.h>

import lexer;
import std;
import types;

using namespace wallhalla_n;
using namespace std;

// ----------- LEXER TESTS -------

namespace
{
  [[nodiscard]] bool checkValue( const token_t &result, const token_t &expected )
  {
    EXPECT_EQ( result->tokenType, expected->tokenType );
    if ( result->tokenType != expected->tokenType )
      return false;

    switch ( result->tokenType )
    {
    case tokenType_t::NUMBER:
      {
        const auto a = static_cast< const number_s & >( *result ).value;
        const auto b = static_cast< const number_s & >( *expected ).value;
        EXPECT_EQ( a, b );
        return a == b;
      }
    case tokenType_t::WORD:
      {
        const auto a = static_cast< const word_s & >( *result ).word;
        const auto b = static_cast< const word_s & >( *expected ).word;
        EXPECT_EQ( a, b );
        return a == b;
      }
    default:
      {
        return true;
      }
    }
  }

  [[nodiscard]] bool checkTokens( const vector< token_t > &result, const vector< token_t > &expected )
  {
    EXPECT_EQ( result.size(), expected.size() );
    if ( result.size() != expected.size() )
      return false;

    for ( u32 i = 0; i < result.size(); ++i )
      if ( !checkValue( result[ i ], expected[ i ] ) )
      {
        println( "failed on index {}", i );
      }

    return true;
  }
} // namespace

TEST( LEXER_TEST, simple_test )
{
  println( "cw: {}", std::filesystem::current_path().string() );
  vector< token_t > tokens = tokenize( "../test/basics/let_a_5.js" );
  vector< token_t > expected;
  expected.push_back( make_unique< word_s >( "let" ) );
  expected.push_back( make_unique< word_s >( "a" ) );
  expected.push_back( make_unique< operator_s >( tokenType_t::ASIGN ) );
  expected.push_back( make_unique< number_s >( 5.0 ) );
  expected.push_back( make_unique< operator_s >( tokenType_t::INSTRUCTION ) );
  EXPECT_TRUE( checkTokens( tokens, expected ) );
}

TEST( LEXER_TEST, member_test )
{
  println( "cw: {}", std::filesystem::current_path().string() );
  vector< token_t > tokens = tokenize( "../test/basics/simple_member_test.js" );
  vector< token_t > expected;
  expected.push_back( make_unique< word_s >( "console" ) );
  expected.push_back( make_unique< operator_s >( tokenType_t::DOT ) );
  expected.push_back( make_unique< word_s >( "log" ) );
  expected.push_back( make_unique< operator_s >( tokenType_t::ROUND_BRACKED_BEGIN ) );
  expected.push_back( make_unique< word_s >( "res" ) );
  expected.push_back( make_unique< operator_s >( tokenType_t::ROUND_BRACKED_END ) );
  expected.push_back( make_unique< operator_s >( tokenType_t::INSTRUCTION ) );
  EXPECT_TRUE( checkTokens( tokens, expected ) );
}
