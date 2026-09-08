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
    PLUS,
    MINUS,
    MULTIPLY,
    DIVIDE,
    ROUND_BRACKED_BEGIN,
    ROUND_BRACKED_END,
    ASIGN,
    INSTRUCTION,
    WORD,
    DOT,
    UNKNOWN
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

  struct operator_s : public tokenBase_s
  {
    operator_s( const tokenType_t value ) :
      tokenBase_s( value )
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

  using token_t = unique_ptr< tokenBase_s >;
} // namespace wallhalla_n

namespace
{
  using namespace wallhalla_n;

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
    result[ '(' ] = 1;
    result[ ')' ] = 1;
    result[ '.' ] = 1;

    return result;
  }

  tokenType_t lookupTokenType( const char c )
  {
    static const std::unordered_map< char, tokenType_t > map = { { '+', tokenType_t::PLUS },
                                                                 { '-', tokenType_t::MINUS },
                                                                 { '/', tokenType_t::DIVIDE },
                                                                 { '*', tokenType_t::MULTIPLY },
                                                                 { '=', tokenType_t::ASIGN },
                                                                 { '+', tokenType_t::PLUS },
                                                                 { '(', tokenType_t::ROUND_BRACKED_BEGIN },
                                                                 { ')', tokenType_t::ROUND_BRACKED_END },
                                                                 { '.', tokenType_t::DOT } };

    const auto it = map.find( c );
    return it != map.end() ? it->second : tokenType_t::UNKNOWN;
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
          result.push_back( make_unique< operator_s >( tokenType_t::INSTRUCTION ) );
        }
        else if ( isOperator( c ) )
          result.push_back( make_unique< operator_s >( lookupTokenType( c ) ) );
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
