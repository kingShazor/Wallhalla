#include "recipe_picker_core.h"

#include <algorithm>
#include <array>
#include <atomic>
#include <condition_variable>
#include <cstdio>
#include <dirent.h>
#include <flat_set>
#include <fstream>
#include <iterator>
#include <mutex>
#include <print>
#include <queue>
#include <stack>
#include <sys/stat.h>
#include <sys/types.h>
#include <thread>
#include <unordered_set>
#include <filesystem>

#include "simple_fuzzy_sorter.h"

using namespace std;
using namespace core_n;

namespace
{
  // maximal file name size of the view - file names need to be shortened
  u32 _maxFileNameSize{ 200 };
  // original fileNames
  vector< string > _fileNames;
  // preIndieces - to save cpu $: idea use hack marks and use them as like a dictory lookup
  // { ab-[1-100] }, {c-[1-80]} -
  // search word: abcx - we can use the 80 candidates from index 1(c).
  // search word: abxc - user added: x in the middle - we still can take 100 candidats from index 0(ab)
  vector< pair< string, vector< u32 > > > _preIndices;
  u32 _viewSize{ 0 };
  // with mid-shortended file names and index to the fullname
  static array< pair< string, u32 >, MAX_FILES_IN_VIEW > _shortenedFileNames;
  // no alloc needed for output c-strings
  static array< const char *, MAX_FILES_IN_VIEW > _out;

  // save remove of cached file data
  void clearFilesData()
  {
    _fileNames.clear();
    _preIndices.clear();
  }
} // namespace

namespace core_n
{
  using std::atomic;

  struct threadPool
  {
    enum
    {
      MAX_THREADS = 8
    };

    vector< thread > threads;
    mutex queue_mutex;
    condition_variable newDir;
    atomic< u32 > threadsRunning{ 0 };
    u32 baseDirSize;
    const unordered_set< string_view > &ignoreDirs;
    flat_set< string > fileTypes;

    stack< string > dirs;

    void scanDir( const string &, vector< string > & );
    void pushDir( const string &, const string_view name );
    inline void addFile( const string &, vector< string > & );
    threadPool( const string &, const unordered_set< string_view > &, flat_set< string > && );
    void waitForThreads();
    inline string fileExtension( const string & ) const;
  };

  inline string threadPool::fileExtension( const string &fileName ) const
  {
    if ( const auto pos = fileName.rfind( '.' ); pos != string::npos )
      return fileName.substr( pos );

    return ""s;
  }

  inline void threadPool::addFile( const string &fileName, vector< string > &fileNames )
  {
    if ( fileTypes.empty() || fileTypes.contains( fileExtension( fileName ) ) )
      fileNames.push_back( fileName.substr( baseDirSize + 1 ) );
  }

  void threadPool::scanDir( const string &dirPath, vector< string > &fileNames )
  {
    DIR *dir = opendir( dirPath.c_str() );
    if ( !dir )
    {
      perror( ( "opendir failed: " + dirPath ).c_str() );
      return;
    }

    struct dirent *entry;
    while ( ( entry = readdir( dir ) ) != nullptr )
    {
      const string_view name{ entry->d_name };

      if ( name == "." || name == ".." )
        continue;

      string fullPath;
      fullPath.reserve( dirPath.size() + name.size() + 1 );
      fullPath.append( dirPath );
      fullPath.push_back( '/' );
      fullPath.append( name );
      if ( entry->d_type != DT_UNKNOWN )
      {
        if ( entry->d_type == DT_REG ) [[likely]]
        {
          addFile( fullPath, fileNames );
          continue;
        }
        else if ( entry->d_type == DT_DIR )
        {
          pushDir( fullPath, name );
          continue;
        }
      }

      struct stat sb;
      if ( lstat( fullPath.c_str(), &sb ) == -1 )
      {
        perror( ( "lstat failed: " + fullPath ).c_str() );
        continue;
      }

      if ( S_ISDIR( sb.st_mode ) )
      {
        pushDir( fullPath, name );
      }
      else if ( S_ISREG( sb.st_mode ) )
      {
        addFile( fullPath, fileNames );
      }
    }

    closedir( dir );
  }

  threadPool::threadPool( const string &baseDir,
                          const unordered_set< string_view > &ignoreDirs,
                          flat_set< string > &&fileTypes ) :
    baseDirSize( baseDir.size() ),
    ignoreDirs( ignoreDirs ),
    fileTypes( std::move( fileTypes ) )
  {
    dirs.push( baseDir );
    const u32 n{ std::min( thread::hardware_concurrency(), static_cast< u32 >( MAX_THREADS ) ) };

    for ( u32 i{ 0 }; i < n; ++i )
    {
      threads.emplace_back(
        [ this  ]
        {
          vector< string > threadFileNames;
          while ( true )
          {
            string dirPath;
            {
              unique_lock< mutex > lock{ this->queue_mutex };
              newDir.wait( lock, [ this ]() { return !dirs.empty() || threadsRunning == 0; } );

              if ( dirs.empty() && threadsRunning == 0 ) [[unlikely]]
              {
                std::ranges::move( threadFileNames, std::back_inserter( _fileNames ) );
                return;
              }

              ++threadsRunning;
              dirPath = dirs.top();
              dirs.pop();
            }
            scanDir( dirPath, threadFileNames );
            --threadsRunning;

            if ( dirs.empty() && threadsRunning == 0 ) [[unlikely]]
              newDir.notify_all();
          }
        } );
    }
  }

  void threadPool::waitForThreads()
  {
    for ( auto &thread : threads )
      thread.join();
  }

  void threadPool::pushDir( const string &fullPath, const string_view dirName )
  {
    if ( ignoreDirs.find( dirName ) != ignoreDirs.end() )
      return;

    lock_guard< mutex > guard( queue_mutex );
    dirs.push( fullPath );

    newDir.notify_one();
  }

  /*
   * Shortens the string, so it fits the max view width (maxSize). It will try
   * middle ellipsis algorithmus first (remove/replaces dirs in the mid). If the this
   * is not possible (filename ist to large), the last chars of the word will be used
   * (Attention: the size is not UTF-8 corrected. todo: this need too be fixed with high prio)
   *
   * @param str       - file name
   * @param maxSize   - maximal view size (utf8-size -arghhh)
   * @param positions - positions of the view (for highlighting)
   */
  [[nodiscard]] string middleEllipsis( const string &str, const u32 maxSize, vector< u32 > *positions )
  {
    if ( str.size() < maxSize )
      return str;

    i32 removesNeeded{ static_cast< i32 >( str.size() - maxSize ) };
    vector< string > splits;
    size_t end = 0;
    u32 start = 0;
    do
    {
      end = str.find( '/', start );
      splits.push_back( str.substr( start, end == string::npos ? string::npos : end - start + 1 ) );
      start = end + 1;
    } while ( end != string::npos );

    bool ellipsizingPossible = true;
    if ( splits.size() > 1 )
    {
      u32 mid{ static_cast< u32 >( splits.size() ) / 2 - 1 };
      const string middleTerm = ".../";
      removesNeeded += middleTerm.size() - splits[ mid ].size();
      splits[ mid ] = middleTerm;
      for ( u32 inc{ 1 }; ellipsizingPossible && removesNeeded > 0; ++inc )
      {
        if ( inc > ( mid + 1 ) || ( ( mid - 1 ) + inc ) >= splits.size() )
        {
          ellipsizingPossible = false;
        }
        else
        {
          const i32 lower{ static_cast< i32 >( mid - inc ) };
          const u32 upper{ mid + inc };

          if ( lower >= 0 )
          {
            const u32 l{ static_cast< u32 >( lower ) };
            removesNeeded -= splits[ l ].size();
            splits[ l ] = "";
          }
          if ( upper < splits.size() - 1 )
          {
            removesNeeded -= splits[ upper ].size();
            splits[ upper ] = "";
          }
        }
      }
      if ( ellipsizingPossible )
      {
        string outStr;
        u32 midPos{ 0 };
        for ( u32 i{ 0 }; i < splits.size(); ++i )
        {
          if ( i == mid )
            midPos = outStr.size();
          outStr.append( splits[ i ] );
        }

        if ( positions )
        {
          const u32 correction{ static_cast< u32 >( str.size() - outStr.size() ) };
          const u32 afterPos{ midPos + static_cast< u32 >( middleTerm.size() ) + correction };
          vector< u32 > tmpPositions;
          for ( u32 pos : *positions )
          {
            if ( pos < midPos )
              tmpPositions.push_back( pos );
            else if ( pos >= afterPos )
            {
              tmpPositions.push_back( pos - correction );
            }
          }

          if ( tmpPositions.size() != positions->size() )
          {
            for ( u32 pos = midPos; pos < midPos + middleTerm.size() - 1; ++pos )
              tmpPositions.push_back( pos );
            ranges::sort( tmpPositions );
          }

          swap( tmpPositions, *positions );
        }
        return outStr;
      }
    }

    // fall back: just remove in the start (filename end - is most important)
    start = str.size() - maxSize;
    string tmp = str.substr( str.size() - maxSize );
    tmp[ 0 ] = '.';
    tmp[ 1 ] = '.';

    if ( positions )
    {
      vector< u32 > tmpPositions;
      bool markMarks = false;
      for ( u32 pos : *positions )
        if ( pos >= start + 2 )
          tmpPositions.push_back( pos - start );
        else
          markMarks = true;

      if ( markMarks )
      {
        tmpPositions.push_back( 0 );
        tmpPositions.push_back( 1 );
        ranges::sort( tmpPositions );
      }

      swap( tmpPositions, *positions );
    }

    return tmp;
  }

  priority_queue< pair< f32, u32 > > getBestResults( const string &prompt, vector< u32 > &foundIndices )
  {
    const u32 maxSize = _preIndices.empty() ? _fileNames.size() : _preIndices.back().second.size();
    priority_queue< pair< f32, u32 > > queue;
    for ( u32 i{ 0 }; i < maxSize; ++i )
    {
      u32 y{ _preIndices.empty() ? i : _preIndices.back().second[ i ] };
      const f32 score{ fzs_get_score( _fileNames[ y ].c_str(), prompt ) };
      if ( score > 0.0 )
      {
        foundIndices.push_back( y );
        if ( queue.size() < MAX_FILES_IN_VIEW )
          queue.push( pair( score, y ) );
        else if ( score < queue.top().first )
        {
          queue.pop();
          queue.push( pair( score, y ) );
        }
      }
    }
    return queue;
  }

  // Escape for shell: word: "bla'foo" wird zu: bla'\'' und in der shell rg -uuu 'bla'\''foo'
  void escapeForShell( string &word )
  {
    size_t pos = 0;
    string rstr = "'\\''"s;
    while( ( pos = word.find( '\'', pos ) ) != string::npos )
    {
      word.replace( pos, 1, rstr );
      pos += rstr.size();
    }
  }

  void callRipGrep( string cmd, const string &baseDir, string word )
  {
    // set current_path to get relative paths to baseDir - less to read for the user
    escapeForShell( word );
    cmd.replace( cmd.find( "{}" ), 2, word );
    // const string cmd = std::format( "rg -o '{}' --column 2>/dev/null | head -n 20000", word );
    // const string cmd = std::format( "rg -o '{}' --column 2>/dev/null | head -n 20000", word );
    std::filesystem::current_path( baseDir );
    // string ignoreFile = baseDir + "/.ignore"s;
    // if ( std::filesystem::is_regular_file( std::filesystem::path( ignoreFile ) ) )
    //   cmd.append( "--ignore-file " + ignoreFile + " " );
    // cmd.append( " 2>/dev/null" );
    // cmd.append( " | head -n 20000" );

    auto file = popen( cmd.c_str(), "r" );
    array< char, 256 > cBuffer;
    std::string buffer;

    while ( fgets( cBuffer.data(), cBuffer.size(), file ) )
      buffer += cBuffer.data();

    istringstream iss( buffer );
    std::string line;

    clearFilesData();
    while ( std::getline( iss, line ) )
    {
      // filename:row:colum:matching_word
      // read the position of the third seperator
      size_t pos = 0;
      for ( u32 i{ 0 }; i < 3; ++i )
        pos = line.find( ':', pos + ( i > 0 ? 1 : 0 ) );

      // Works also with utf8 :-D 'Änderung'
      string_view lineSV = line;
      string_view greppedWord = lineSV.substr( pos + 1 );
      const u32 greppedWordSize = greppedWord.size();

      line = line.substr( 0, pos + 1 );
      line += format( "{}", greppedWordSize );
      // format now: filename:row:column:matching_word_size in Byte
      _fileNames.push_back( line );
    }
  }
} // namespace core_n

namespace
{
  flat_set< string > considerFileTypes( const char *fileTypes)
  {
    flat_set< string > consideredFileTypes;
    if ( fileTypes )
    {
      string_view sv = fileTypes;
      size_t pos = 0;
      while ( pos != string_view::npos )
      {
        const auto startPos = pos;
        pos = sv.find( ',', pos );
        // pos - startPos = don't take the ','
        const auto fileType = sv.substr( startPos, pos == string_view::npos ? pos : pos - startPos ); 
        if ( fileType.size() >= 1 )
          consideredFileTypes.insert( "."s + fileType );
        if ( pos != string_view::npos )
          ++pos;
      }
    }

    return consideredFileTypes;
  }
}

// -------- C-Interface ----------


extern "C"
{
  result_t *findFileNames( const char *baseDir, const char *fileTypes, const unsigned int maxSize )
  {
    _maxFileNameSize = static_cast< u32 >( maxSize );
    // fileNames.resize( 495000 );
    // todo bessere heuristik wieder weg

    using namespace std::chrono;
    string baseDirStr = baseDir;
    ifstream file{ baseDirStr + "/.ignore" };
    unordered_set< string_view > ignoreDirs;
    vector< string > iDir;
    if ( file.is_open() )
    {
      string line;
      while ( std::getline( file, line ) )
        iDir.push_back( line );
      for ( const auto &str : iDir )
        ignoreDirs.insert( string_view( str ) );
    }


    clearFilesData();
    threadPool search( baseDirStr, ignoreDirs, considerFileTypes( fileTypes ) );
    search.waitForThreads();

    // auto data = new const char *[ fileNames.size() ];
    _viewSize = min( _out.max_size(), _fileNames.size() );
    for ( u32 i{ 0 }; i < _viewSize; ++i )
    {
      _shortenedFileNames[ i ] = pair( middleEllipsis( _fileNames[ i ], _maxFileNameSize, nullptr ), i );
      _out[ i ] = _shortenedFileNames[ i ].first.c_str();
    }
    // const auto end1 = high_resolution_clock::now();
    return new result_t{ .viewFileNames = _out.data(),
                         .viewFileNamesSize = _viewSize,
                         .filteredFileNamesSize = static_cast< u32 >( _fileNames.size() ),
                         .fileNamesSize = static_cast< u32 >( _fileNames.size() ),
                         .positions = nullptr };
  }

  size_t syncPreIndices( const string &prompt )
  {
    size_t lastPos = 0;
    string_view promptV = prompt;
    for ( size_t i{ 0 }; i < _preIndices.size(); ++i )
    {
      const string &keyWord = _preIndices[ i ].first;
      if ( !promptV.starts_with( keyWord ) )
      {
        while ( _preIndices.size() > i )
          _preIndices.pop_back();
        break;
      }

      lastPos += keyWord.size();
      promptV = promptV.substr( keyWord.size() );
    }

    return lastPos;
  }

  result_t *filterFileNames( const char *promptCStr )
  {
    string prompt = promptCStr;
    // remember: priority_queue using '<'-operator which means max-Heap

    // todo we can just generate the score again for them
    vector< u32 > foundIndices;
    // score with
    // priority_queue< pair< f32, u32 > > queue;
    const auto nextPos = syncPreIndices( prompt );

    auto queue = getBestResults( prompt, foundIndices );

    if ( nextPos < prompt.size() )
      _preIndices.push_back( pair( prompt.substr( nextPos ), foundIndices ) );

    // todo zusammenfassen
    _viewSize = min( _out.max_size(), queue.size() );
    vector< u32 > resultIndices( _viewSize, 0 );
    // auto data = new const char *[ fileNames.size() ];
    for ( u32 i{ 0 }; i < _viewSize; ++i )
    {
      resultIndices[ i ] = queue.top().second;
      queue.pop();
    }
    reverse( resultIndices.begin(), resultIndices.end() );

    auto out_pos = new positions_t[ resultIndices.size() ];
    for ( u32 i{ 0 }; i < resultIndices.size(); ++i )
    {
      const string &fileName{ _fileNames[ resultIndices[ i ] ] };
      auto positions = fzs_get_positions( fileName, prompt );
      _shortenedFileNames[ i ] = pair( middleEllipsis( fileName, _maxFileNameSize, &positions ), resultIndices[ i ] );
      _out[ i ] = _shortenedFileNames[ i ].first.c_str();
      out_pos[ i ].data = new u32[ positions.size() ];
      out_pos[ i ].size = positions.size();
      for ( u32 y{ 0 }; y < positions.size(); ++y )
        out_pos[ i ].data[ y ] = positions[ y ];
    }

    return new result_t{ .viewFileNames = _out.data(),
                         .viewFileNamesSize = _viewSize,
                         .filteredFileNamesSize = static_cast< u32 >( foundIndices.size() ),
                         .fileNamesSize = static_cast< u32 >( _fileNames.size() ),
                         .positions = out_pos };
  }

  /*
   * Return the original file name for the result index.
   * This is necessary because long names will be shortened (see middleEllepsis).
   */
  const char *getFullFileName( const unsigned int index )
  {
    if ( index >= _viewSize )
      return nullptr;
    return _fileNames[ _shortenedFileNames[ index ].second ].c_str();
  }

  // Alle gefundenen Dateinamen liefern
  result_t *getAllFileNames()
  {
    if ( _preIndices.empty() )
    {
      const char **viewFileNames{ new const char *[ _fileNames.size() ] };
      u32 i{ 0 };
      for ( const auto &fileName : _fileNames )
        viewFileNames[ i++ ] = fileName.c_str();

      return new result_t{ .viewFileNames = viewFileNames,
                           .viewFileNamesSize = static_cast< u32 >( _fileNames.size() ),
                           .filteredFileNamesSize = static_cast< u32 >( _fileNames.size() ),
                           .fileNamesSize = static_cast< u32 >( _fileNames.size() ),
                           .positions = nullptr };
    }
    else
    {
      const auto &cachedIndices = _preIndices.back().second;
      const char **viewFileNames{ new const char *[ cachedIndices.size() ] };
      u32 i{ 0 };
      for ( const auto &cachedIndex : cachedIndices )
        viewFileNames[ i++ ] = _fileNames[ cachedIndex ].c_str();

      return new result_t{ .viewFileNames = viewFileNames,
                           .viewFileNamesSize = static_cast< u32 >( cachedIndices.size() ),
                           .filteredFileNamesSize = static_cast< u32 >( cachedIndices.size() ),
                           .fileNamesSize = static_cast< u32 >( _fileNames.size() ),
                           .positions = nullptr };
    }
  }

  void deleteFileNameResult( result_t *outStruct )
  {
    if ( auto positions = outStruct->positions; positions )
    {
      for ( u32 i{ 0 }; i < outStruct->viewFileNamesSize; ++i )
        delete[] positions[ i ].data;
      delete positions;
    }

    if ( auto viewFileNamesPtr = outStruct->viewFileNames; viewFileNamesPtr != _out.data() )
      delete[] viewFileNamesPtr;
    delete outStruct;
  }

  result_t *grepWord( const char *cmd, const char *baseDir, const char *word, const unsigned int maxSize )
  {
    _maxFileNameSize = static_cast< u32 >( maxSize );
    callRipGrep( cmd, baseDir, word );
    _viewSize = std::min( _out.size(), _fileNames.size() );
    for ( u32 i{ 0 }; i < _viewSize; ++i )
    {
      _shortenedFileNames[ i ] = pair( middleEllipsis( _fileNames[ i ], _maxFileNameSize, nullptr ), i );
      _out[ i ] = _shortenedFileNames[ i ].first.c_str();
    }

    return new result_t{ .viewFileNames = _out.data(),
                         .viewFileNamesSize = _viewSize,
                         .filteredFileNamesSize = static_cast< u32 >( _fileNames.size() ),
                         .fileNamesSize = static_cast< u32 >( 0 ),
                         .positions = nullptr };
  }

  void fillFileNames( const char **files, const unsigned int size, const unsigned int maxSize )
  {
    clearFilesData();
    const u32 usize = static_cast< u32 >( size );
    _fileNames.reserve( usize );
    for ( u32 i = 0; i < usize; ++i )
      _fileNames.push_back( files[ i ] );

    _maxFileNameSize = maxSize;
    _viewSize = std::min( _out.size(), _fileNames.size() );
    for ( u32 i{ 0 }; i < _viewSize; ++i )
      _shortenedFileNames[ i ] = pair( middleEllipsis( _fileNames[ i ], _maxFileNameSize, nullptr ), i );
  }

  result_t *getFirstShortenedFileNames()
  {
    _viewSize = std::min( _out.size(), _fileNames.size() );
    for ( u32 i{ 0 }; i < _viewSize; ++i )
      _out[ i ] = _shortenedFileNames[ i ].first.c_str();

    return new result_t{ .viewFileNames = _out.data(),
                         .viewFileNamesSize = _viewSize,
                         .filteredFileNamesSize = static_cast< u32 >( _fileNames.size() ),
                         .fileNamesSize = static_cast< u32 >( 0 ),
                         .positions = nullptr };
  }
}
