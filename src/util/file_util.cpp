module;
#include <cerrno>
export module file;
import std;
import types;

using namespace std;

export namespace wallhalla_n
{
  enum class fileMode_t : u8
  {
    READ = 1
  };

  const char *toCStr( const fileMode_t mode )
  {
    switch ( mode )
    {
    case fileMode_t::READ:
      return "r";
    }

    return "";
  }

  struct fileError_s
  {
    i32 code;
  };

  std::string getErrorMsg( const fileError_s &error )
  {
    switch ( error.code )
    {
    case EACCES:
      return "Permission denied";
    case ENOENT:
      return "No such file or directory";
    case EEXIST:
      return "File already exists";
    default:
      return std::format( "Native error code: {}", error.code );
    }
  }

  struct file_s
  {
    string fileName;
    fileMode_t mode;
    FILE *handle;
  };

  struct fileGuard_s
  {
    file_s file;

  public:
    fileGuard_s( file_s &&file ) :
      file( std::move( file ) )
    {
    }

    ~fileGuard_s()
    {
      if ( file.handle )
        std::fclose( file.handle );
    }
  };

  std::expected< fileGuard_s, fileError_s > openFile( const std::string &str, const fileMode_t mode )
  {
    FILE *file = std::fopen( str.c_str(), toCStr( mode ) );
    if ( file )
      return file_s{ .fileName = str, .mode = mode, .handle = file };

    fileError_s error{ .code = errno };
    return std::unexpected< fileError_s >( error );
  }
} // namespace wallhalla_n
