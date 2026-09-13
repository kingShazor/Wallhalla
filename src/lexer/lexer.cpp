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

  struct numberCheck_s
  {
    bool isNumber;
    bool useFloating;
    i32 base;

    numberCheck_s( const bool isNumber, const bool useFloating = false, const i32 base = 10 ) :
      isNumber( isNumber ),
      useFloating( useFloating ),
      base( base )
    {
    }

    operator bool() const
    {
      return isNumber;
    }
  };

  numberCheck_s isDifferentNumberSystem( const string &word )
  {
    if ( word.size() < 3 )
      return false;

    println( "isDifferentNumberSystem: {}", word );
    // HEX
    if ( const char c = word[ 1 ]; c == 'x' || c == 'X' )
    {
      println( "check HEX!" );
      for ( u32 i = 2; i < word.size(); ++i )
      {
        if ( !isHexValue( word[ i ] ) )
          return false;
      }
      return numberCheck_s( true, false, 16 );
    }
    // OKTAL
    else if ( c == 'o' || c == 'O' )
    {
      println( "check oktal!" );
    }
    // BINARY
    else if ( c == 'b' || c == 'B' )
    {
      println( "check binary!" );
      for ( u32 i = 2; i < word.size(); ++i )
        if ( const char ch = word[ i ]; !( ch == '0' || ch == '1' ) )
        {
          println( "check binary failed {}", ch );
          return false;
        }

      return numberCheck_s( true, false, 2 );
    }
    else
    {
      println( "not valid {}", c );
    }

    return false;
  }

  numberCheck_s isNumber( const string &word )
  {
    if ( word.size() > 1 && word.front() == '0' && word[ 1 ] != '.' )
      return isDifferentNumberSystem( word );
    u8 dotCount = 0;
    for ( u32 i = 0; i < word.size(); ++i )
    {
      char c = word[ i ];
      if ( isNumber( c ) )
        continue;
      if ( c == '.' )
      {
        if ( ++dotCount > 1 )
          return false;
      }
      // parse exponent
      else if ( ( c == 'e' || c == 'E' ) && i > 0 && isNumber( word[ i - 1 ] ) )
      {
        ++i;
        if ( i >= word.size() )
          return false;
        c = word[ i ];
        // todo don't allow double exponent
        return numberCheck_s( isNumber( word.substr( ( c == '+' || c == '-' ) ? i + 1 : i ) ), true );
      }
      else
        return false;
    }
    // println( "is number {}", word );
    return numberCheck_s{ dotCount <= 1 && word.size() - dotCount > 0, dotCount > 0 };
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

  f64 convertToNumber( const string &word, const numberCheck_s &res )
  {
    if ( res.useFloating )
      return std::stod( word );

    return static_cast< f64 >( std::stol( word, nullptr, res.base ) );
  }
} // namespace

export namespace wallhalla_n
{
  std::vector< token_t > buildTokens( const string &content )
  {
    std::vector< token_t > result;
    string word;

    bool parseNumber = false;
    for ( u32 i = 0; i < content.size(); ++i )
    {
      const char c = content[ i ];
      println( "c: {}, word {}, isOperator {}, parseNumber {}", c, word, isOperator( c ), parseNumber );
      if ( const bool addInstructin = ( c == '\n' || c == ';' );
           std::isspace( static_cast< u8 >( c ) ) || addInstructin ||
           // allowing floating numbers and exponent
           ( isOperator( c ) && !( parseNumber && ( c == '.' || c == '+' || c == '-' ) ) ) )
      {
        if ( !word.empty() )
        {
          if ( const auto res = isNumber( word ); res )
          {
            result.push_back( make_unique< number_s >( convertToNumber( word, res ) ) );
            parseNumber = false;
          }
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
        {
          if ( c == '.' && i + 1 < content.size() && isNumber( content[ i + 1 ] ) )
          {
            parseNumber = true;
            word.push_back( c );
            continue;
          }
          result.push_back( make_unique< operator_s >( lookupTokenType( c ) ) );
        }
      }
      else
      {
        word.push_back( c );
        if ( word.size() == 1 )
          parseNumber = isNumber( c );
      }
    }

    if ( !word.empty() )
    {
      if ( parseNumber )
      {
        const auto res = isNumber( word );
        result.push_back( make_unique< number_s >( convertToNumber( word, res ) ) );
      }
      else
        result.push_back( make_unique< word_s >( word ) );
      result.push_back( make_unique< operator_s >( tokenType_t::INSTRUCTION ) );
    }
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
