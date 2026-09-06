export module lexer;

import std;
import file_reader;
import file;

using namespace std;

export namespace wallhalla_n
{
  void tokenize( const string &fileName )
  {
    println( "tokenize {}", fileName );
    auto res = openFile( fileName, fileMode_t::READ );
    if ( !res )
    {
      std::println( "can't open file '{}'. Error message: '{}'", fileName, getErrorMsg( res.error() ) );
      exit( 1 );
    }

    fileReader_c reader( std::move( *res ) );
    print( "{}:\n{}", fileName, reader.getFileData() );
  }

} // namespace asafaw_n
