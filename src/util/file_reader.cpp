export module file_reader;
import file;
import std;
import types;

using namespace std;

export namespace wallhalla_n
{
  class fileReader_c
  {
    fileGuard_s &&fileGuard;
    std::string fileData;

    void readFile()
    {
      array< char, 256 > cBuffer;
      auto &file = fileGuard.file;
      while ( const u64 read = std::fread( cBuffer.data(), sizeof( char ), cBuffer.size(), file.handle ) )
        fileData += string_view( cBuffer.data(), read );
    }

    public:
      fileReader_c( fileGuard_s &&fileGuard ) :
        fileGuard( std::move( fileGuard ) )
    {
      const auto &file = fileGuard.file;
      if ( file.mode != fileMode_t::READ )
      {
        println( "Wrong file access mode. Expected a read access got {}", toCStr( file.mode ) );
        exit( 1 );
      }
      readFile();
    }


    // erstmal mit copy bei der Verwendung spaeter verfeinern
    const string &getFileData() const
    {
      return fileData;
    }
  };
}
