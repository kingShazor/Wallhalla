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
    {
      for ( const auto &item : result )
        println( "found token: {}", item->dump() );

      return false;
    }

    for ( u32 i = 0; i < result.size(); ++i )
      if ( !checkValue( result[ i ], expected[ i ] ) )
      {
        println( "failed on index {}", i );
      }

    return true;
  }

  template< typename... TOKENS >
  vector< token_t > buildTokens( TOKENS &&...tokens )
  {
    vector< token_t > result;
    result.reserve( sizeof...( TOKENS ) );

    ( result.emplace_back( make_unique< TOKENS >( std::forward< TOKENS >( tokens ) ) ), ... );

    return result;
  }

  const filesystem::path testDir = TEST_DATA_DIR;

} // namespace

TEST( LEXER_TEST, simple_test )
{
  vector< token_t > tokens = tokenize( testDir / "basics/let_a_5.js" );
  vector< token_t > expected = buildTokens( word_s( "let" ),
                                            word_s( "a" ),
                                            operator_s( tokenType_t::ASIGN ),
                                            number_s( 5.0 ),
                                            operator_s( tokenType_t::INSTRUCTION ) );
  EXPECT_TRUE( checkTokens( tokens, expected ) );
}

TEST( LEXER_TEST, member_test )
{
  vector< token_t > tokens = tokenize( testDir / "basics/simple_member_test.js" );
  vector< token_t > expected = buildTokens( word_s( "console" ),
                                            operator_s( tokenType_t::DOT ),
                                            word_s( "log" ),
                                            operator_s( tokenType_t::ROUND_BRACKED_BEGIN ),
                                            word_s( "res" ),
                                            operator_s( tokenType_t::ROUND_BRACKED_END ),
                                            operator_s( tokenType_t::INSTRUCTION ) );
  EXPECT_TRUE( checkTokens( tokens, expected ) );
}

TEST( LEXER_TEST, pi )
{
  vector< token_t > tokens = wallhalla_n::buildTokens( "let b = 3.14159" );
  vector< token_t > expected = buildTokens( word_s( "let" ),
                                            word_s( "b" ),
                                            operator_s( tokenType_t::ASIGN ),
                                            number_s( 3.14159 ),
                                            operator_s( tokenType_t::INSTRUCTION ) );
  EXPECT_TRUE( checkTokens( tokens, expected ) );
}

TEST( LEXER_TEST, floating_1 )
{
  vector< token_t > tokens = wallhalla_n::buildTokens( "let b = 0.5" );
  vector< token_t > expected = buildTokens( word_s( "let" ),
                                            word_s( "b" ),
                                            operator_s( tokenType_t::ASIGN ),
                                            number_s( 0.5 ),
                                            operator_s( tokenType_t::INSTRUCTION ) );
  EXPECT_TRUE( checkTokens( tokens, expected ) );
}

TEST( LEXER_TEST, floating_2 )
{
  vector< token_t > tokens = wallhalla_n::buildTokens( "let b = .5" );
  vector< token_t > expected = buildTokens( word_s( "let" ),
                                            word_s( "b" ),
                                            operator_s( tokenType_t::ASIGN ),
                                            number_s( 0.5 ),
                                            operator_s( tokenType_t::INSTRUCTION ) );
  EXPECT_TRUE( checkTokens( tokens, expected ) );
}

TEST( LEXER_TEST, floating_3 )
{
  vector< token_t > tokens = wallhalla_n::buildTokens( "let b = 5." );
  vector< token_t > expected = buildTokens( word_s( "let" ),
                                            word_s( "b" ),
                                            operator_s( tokenType_t::ASIGN ),
                                            number_s( 5 ),
                                            operator_s( tokenType_t::INSTRUCTION ) );
  EXPECT_TRUE( checkTokens( tokens, expected ) );
}
