module;
#include <cctype>
#include <utility>
export module lexer;

import std;
import file_reader;
import file;
import types;

using namespace std;
using namespace wallhalla_n;

constexpr u32 CHAR_SIZE = 256;

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

} // namespace wallhalla_n

namespace
{
  char lookupChar( const tokenType_t );
} // namespace

export namespace wallhalla_n
{
  struct tokenBase_s
  {
    tokenType_t tokenType;

    tokenBase_s( const tokenType_t type ) :
      tokenType( type )
    {
    }

    virtual ~tokenBase_s() = default;
    virtual string dump() const = 0;
  };

  struct number_s : public tokenBase_s
  {
    f64 value;

    number_s( const f64 value ) :
      tokenBase_s( tokenType_t::NUMBER ),
      value( value )
    {
    }

    virtual string dump() const final
    {
      return format( "number: {}", value );
    }
  };

  struct bigInt_s : public tokenBase_s
  {
    i64 value;

    bigInt_s( const i64 value ) :
      tokenBase_s( tokenType_t::NUMBER ),
      value( value )
    {
    }

    virtual string dump() const final
    {
      return format( "bigInt: {}", value );
    }
  };

  struct operator_s : public tokenBase_s
  {
    operator_s( const tokenType_t value ) :
      tokenBase_s( value )
    {
    }

    virtual string dump() const final
    {
      return format( "operator - tokenType: {} char {}", std::to_underlying( tokenType ), lookupChar( tokenType ) );
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

    virtual string dump() const final
    {
      return format( "word: {}", word );
    }
  };

  using token_t = unique_ptr< tokenBase_s >;
} // namespace wallhalla_n

namespace
{
  using namespace wallhalla_n;

  bool isHexValue( const char c )
  {
    const static auto hexValues = []() -> vector< u16 >
    {
      vector< u16 > res( CHAR_SIZE, 0 );
      res[ 'a' ] = true;
      res[ 'b' ] = true;
      res[ 'c' ] = true;
      res[ 'd' ] = true;
      res[ 'e' ] = true;
      res[ 'f' ] = true;
      res[ 'A' ] = true;
      res[ 'B' ] = true;
      res[ 'C' ] = true;
      res[ 'D' ] = true;
      res[ 'E' ] = true;
      res[ 'F' ] = true;
      return res;
    }();

    return hexValues[ static_cast< u8 >( c ) ] == 1;
  }

  bool isNumber( const char c )
  {
    return c >= '0' && c <= '9';
  }

  struct lexerError_s
  {
    string error;
    u32 position;
  };

  bool nextIsNumber( const u32 index, const string &content )
  {
    return index +1 < content.size() && isNumber( content[ index +1 ] );
  }

  u32 nextIsExponent( const u32 index, const string &content )
  {
    if ( nextIsNumber( index, content ) )
      return 1;
    if ( index + 2 >= content.size() )
      return 0;
    const char next = content[ index +1 ];
    return ( next == '+' || next == '-' ) && nextIsNumber( index +1, content) ? 2 : 0;
  }

  std::expected< f64, lexerError_s > parseDifferentNumberSystem( const string &content, u32 &i )
  {
    u32 startIndex = i - 1;

    // HEX
    if ( const char c = content[ i ]; c == 'x' || c == 'X' )
    {
      println( "check HEX!" );
      for ( ++i; i < content.size(); ++i )
      {
        if ( !isHexValue( content[ i ] ) )
          break;
      }
      const u32 size = i - startIndex;
      if ( size < 3 )
        return unexpected( lexerError_s{ .error = "doesn't match hex number", .position = startIndex } );

      return static_cast< f64 >( std::stol( content.substr( startIndex, size ), nullptr, 16 ) );
    }
    // OKTAL
    else if ( c == 'o' || c == 'O' )
    {
      // todo impl
      println( "check oktal!" );
    }
    // BINARY
    else if ( c == 'b' || c == 'B' )
    {
      println( "check binary!" );
      for ( ++i; i < content.size(); ++i )
        if ( const char ch = content[ i ]; !( ch == '0' || ch == '1' ) )
          break;

      const u32 size = i - startIndex;
      if ( size < 3 )
        return unexpected( lexerError_s{ .error = "doesn't match binary number", .position = startIndex } );

      return static_cast< f64 >( std::stol( content.substr( startIndex, size ), nullptr, 2 ) );
    }
    else
    {
      println( "not valid {}", c );
    }

    return unexpected( lexerError_s{ .error = "miss matching different system number", .position = startIndex } );
  }

  std::expected< f64, lexerError_s > parseNumber( const string &content, u32 &i )
  {
    if ( content[ i ] == '0' && i + 1 < content.size() && content[ i + 1 ] != '.' )
      return parseDifferentNumberSystem( content, ++i );
    u8 dotCount = 0;

    u32 startIndex = i;
    bool parseExponent = false;
    for ( ; i < content.size(); ++i )
    {
      char c = content[ i ];
      if ( isNumber( c ) )
        continue;
      if ( c == '.' )
      {
        if ( ++dotCount > 1 )
          return unexpected( lexerError_s{ .error = "To many dots in floating number", .position = i } );
        ;
      }
      // parse exponent
      else if ( c == 'e' || c == 'E' )
      {
        if ( parseExponent || !isNumber( content[ i - 1 ] ) )
          return unexpected( lexerError_s{ .error = "Invalid number", .position = startIndex } );
        if ( const u32 exponentStartIndex = nextIsExponent( i, content ); exponentStartIndex > 0 )
          i += exponentStartIndex -1;
        else
          return unexpected( lexerError_s{ .error = "Exponent number has no exponent value {}", .position = i } );

        parseExponent = true;
      }
      else
        break;
    }
    const u32 size = i - startIndex;
    println( "is number {}", content.substr( startIndex, size ) );
    return std::stod( content.substr( startIndex, size ) );
  }

  vector< pair< char, tokenType_t > > getOperatorVec()
  {
    static const vector< pair< char, tokenType_t > > vec = { { '+', tokenType_t::PLUS },
                                                             { '-', tokenType_t::MINUS },
                                                             { '/', tokenType_t::DIVIDE },
                                                             { '*', tokenType_t::MULTIPLY },
                                                             { '=', tokenType_t::ASIGN },
                                                             { '(', tokenType_t::ROUND_BRACKED_BEGIN },
                                                             { ')', tokenType_t::ROUND_BRACKED_END },
                                                             { '.', tokenType_t::DOT } };
    return vec;
  }

  tokenType_t lookupTokenType( const char c )
  {
    static const auto map = []() -> unordered_map< char, tokenType_t >
    {
      unordered_map< char, tokenType_t > result;
      for ( const auto &pair : getOperatorVec() )
        result.insert( pair );

      return result;
    }();

    const auto it = map.find( c );
    return it != map.end() ? it->second : tokenType_t::UNKNOWN;
  }

  char lookupChar( const tokenType_t tokenType )
  {
    static const auto map = []() -> unordered_map< tokenType_t, char >
    {
      unordered_map< tokenType_t, char > result;
      for ( const auto [ c, tt ] : getOperatorVec() )
        result[ tt ] = c;

      return result;
    }();

    const auto it = map.find( tokenType );
    return it != map.end() ? it->second : '?';
  }

  bool isOperator( const char c )
  {
    static auto operators = []() -> vector< u16 >
    {
      vector< u16 > result( sizeof( u8 ) * CHAR_SIZE, 0 );
      for ( const char ch : getOperatorVec() | std::views::keys )
        result[ static_cast< u8 >( ch ) ] = 1;

      result[ ';' ] = 1;
      return result;
    }();
    return operators[ static_cast< u8 >( c ) ] != 0;
  }

  bool isInstruction( const char c )
  {
    return c == '\n' || c == ';';
  }
} // namespace

export namespace wallhalla_n
{
  std::vector< token_t > buildTokens( const string &content )
  {
    std::vector< token_t > result;

    for ( u32 i = 0; i < content.size(); ++i )
    {
      const char c = content[ i ];
      if ( isNumber( c ) || ( c == '.' && i + 1 < content.size() && isNumber( content[ i + 1 ] ) ) )
      {
        const auto res = parseNumber( content, i );
        if ( !res )
        {
          std::println( "error: {}, index {}", res.error().error, res.error().position );

          return {};
        }
        result.push_back( make_unique< number_s >( *res ) );
        --i;
        // todo !happy path
      }
      else if ( std::isalpha( c ) ) // todo + locale?
      {
        u32 startIndex = i;
        for ( ++i; i < content.size(); ++i )
          if ( !std::isalpha( content[ i ] ) )
            break;

        const u32 size = i - startIndex;
        result.push_back( make_unique< word_s >( content.substr( startIndex, size ) ) );
        --i;
      }
      else if ( isInstruction( c ) )
      {
        if ( !isInstruction( content[ i - 1 ] ) )
          result.push_back( make_unique< operator_s >( tokenType_t::INSTRUCTION ) );
      }
      else if ( isOperator( c ) )
        result.push_back( make_unique< operator_s >( lookupTokenType( c ) ) );

    }

    if ( !result.empty() && result.back()->tokenType != tokenType_t::INSTRUCTION )
      result.push_back( make_unique< operator_s >( tokenType_t::INSTRUCTION ) );
    return result;
  }

  std::vector< token_t > tokenize( const filesystem::path &file )
  {
    const string fileName = file.string();
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
