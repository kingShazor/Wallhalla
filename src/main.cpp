import std;
import lexer;
import types;

using namespace std;
using namespace wallhalla_n;

int main( const i32 argc, char **argv )
{
  println( "start engine {}!", "Wallhalla" );
  for ( i32 i = 1; i < argc; ++i )
  {
    const auto tokens = tokenize( argv[ i ] );
    println( "token count: {}", tokens.size() );
    for ( const auto &token : tokens )
    {
      switch ( token->tokenType )
      {
      case tokenType_t::NUMBER:
        println( "Number: {}", static_cast< const number_s * >( token.get() )->value );
        break;
      case tokenType_t::WORD:
        println( "Word: '{}'", static_cast< word_s * >( token.get() )->word );
        break;
      default:
        {
          const auto operatorToken = static_cast< const operator_s * >( token.get() );
          println( "Operator: {}", std::to_underlying( operatorToken->tokenType ) );
        }
      }
    }
  }
  return 0;
}
