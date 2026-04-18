#pragma once

#include <string>
#include <vector>

#include "types.h"

using std::string;
using std::vector;

namespace core_n
{
  enum
  {
    MAX_FILES_IN_VIEW = 200
  };

  [[nodiscard]] string middleEllipsis( const string &str, const u32 maxSize, vector< u32 > *positions = nullptr );
} // namespace core_n

extern "C"
{
  // positions of matching char within the filename
  typedef struct
  {
    unsigned int *data;
    unsigned int size;
  } positions_t;

  typedef struct
  {
    // File Names to print (max. 200)
    const char **viewFileNames;
    unsigned int viewFileNamesSize;

    // filtered based on prompt
    unsigned int filteredFileNamesSize;
    // found file names size
    unsigned int fileNamesSize;
    // n = viewFileNamesSize * positions_t
    positions_t *positions;
  } result_t;

  result_t *grepWord( const char *cmd, const char *baseDir, const char *word, const unsigned int maxSize );

  result_t *getAllFileNames();
  result_t *getFirstShortenedFileNames();
  const char *getFullFileName( const unsigned int index );
  void deleteFileNameResult( result_t *outStruct );
  result_t *filterFileNames( const char *promptCStr );
  result_t *findFileNames( const char *baseDir, const char *filetypes, const unsigned int maxSize );
  void fillFileNames( const char **files, const unsigned int size, const unsigned int maxSize );
}
