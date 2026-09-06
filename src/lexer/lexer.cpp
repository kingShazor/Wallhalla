module;
#include <cctype>
export module lexer;

import std;
import file_reader;
import file;
import types;

using namespace std;

export namespace wallhalla_n
{
  enum class tokenType_t : u8
  {
    NUMBER,
    OPERATOR,
    WORD
  };

  struct tokenBase_s
  {
    tokenType_t tokenType;

    tokenBase_s( const tokenType_t type ) :
      tokenType( type )
    {
    }

    virtual ~tokenBase_s() = default;
  };

  struct number_s : public tokenBase_s
  {
    f64 value;

    number_s( const f64 value ) :
      tokenBase_s( tokenType_t::NUMBER ),
      value( value )
    {
    }
  };

  enum class operator_t : u8
  {
    PLUS,
    MINUS,
    MULTIPLY,
    DIVIDE,
    ROUND_BRACKED_BEGIN,
    ROUND_BRACKED_END,
    INSTRUCTION
  };

  struct operator_s : public tokenBase_s
  {
    operator_t value;
    char sign;

    operator_s( const operator_t value, const char c ) :
      tokenBase_s( tokenType_t::OPERATOR ),
      value( value ),
      sign( c )
    {
    }
  };

  // keywords, methods, functions or variable names
  struct word_s : public tokenBase_s
  {
    string word;

    word_s( const string &word ) :
      tokenBase_s( tokenType_t::WORD ),
      word( word )
    {
    }
  };

} // namespace wallhalla_n

namespace
{
  using namespace wallhalla_n;
  using token_t = unique_ptr< tokenBase_s >;

  bool isNumber( const string &word )
  {
    u8 dotCount = 0;
    for ( const char c : word )
    {
      if ( c >= '0' && c <= '9' )
        continue;
      if ( c == '.' )
      {
        if ( ++dotCount > 1 )
          return false;
      }
      else
        return false;
    }

    // println( "is number {}", word );
    return dotCount <= 1 && word.size() - dotCount > 0;
  }

  std::vector< u16 > operatorVec()
  {
    std::vector< u16 > result( sizeof( u8 ) * 256, 0 );
    result[ '+' ] = 1;
    result[ '-' ] = 1;
    result[ '/' ] = 1;
    result[ '*' ] = 1;
    result[ ';' ] = 1;
    result[ '=' ] = 1;
    result[ '.' ] = 1;

    return result;
  }

  bool isOperator( const char c )
  {
    static auto operators = operatorVec();
    return operators[ static_cast< u8 >( c ) ] != 0;
  }

  std::vector< token_t > buildTokens( const string &content )
  {
    std::vector< token_t > result;
    string word;

    for ( u32 i = 0; i < content.size(); ++i )
    {
      const char c = content[ i ];
      println( "c: {}, word {}, isOperator {}", c, word, isOperator( c ) );
      if ( const bool addInstructin = ( c == '\n' || c == ';' );
           std::isspace( static_cast< u8 >( c ) ) || addInstructin || isOperator( c ) )
      {
        if ( !word.empty() )
        {
          if ( isNumber( word ) )
            result.push_back( make_unique< number_s >( std::stod( word ) ) );
          else
            result.push_back( make_unique< word_s >( word ) );

          println( "got word {}", word );
          word.clear();
        }
        if ( addInstructin )
        {
          if ( c == ';' && i < content.size() && content[ i + 1 ] == '\n' )
            ++i;
          result.push_back( make_unique< operator_s >( operator_t::INSTRUCTION, ';' ) );
        }
        else if ( isOperator( c ) )
          result.push_back( make_unique< operator_s >( operator_t::PLUS, c ) );
      }
      else
        word.push_back( c );
    }

    return result;
  }
} // namespace

export namespace wallhalla_n
{
  std::vector< token_t > tokenize( const string &fileName )
  {
    println( "tokenize {}", fileName );
    auto res = openFile( fileName, fileMode_t::READ );
    if ( !res )
    {
      println( "can't open file '{}'. Error message: '{}'", fileName, getErrorMsg( res.error() ) );
      exit( 1 );
    }

    fileReader_c reader( std::move( *res ) );
    const auto readRes = reader.readFile();
    if ( !readRes )
    {
      println( "can't open file '{}'. Error message: '{}'", fileName, getErrorMsg( readRes.error() ) );
      exit( 1 );
    }

    const string &content = *readRes;
    print( "{}:\n{}", fileName, content );

    return buildTokens( content );
  }

} // namespace wallhalla_n
