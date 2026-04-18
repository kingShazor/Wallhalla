-- Based on parts of telescope-fzf-native.nvim (MIT License)
-- Original author: Simon Hauser

local ffi = require 'ffi'
local library_path = (function()
  local dirname = string.sub(debug.getinfo(1).source, 2, #'/native.lua' * -1)
  if package.config:sub(1, 1) == '\\' then
    return dirname .. '../../build/librecipe_picker.dll'
  else
    -- return dirname .. '../../release_builds/librecipe_picker.so'
    return dirname .. '../../build/librecipe_picker.so'
  end
end)()
local c_interface = ffi.load(library_path)

ffi.cdef [[
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
    positions_t *positions;
  } result_t;

  result_t *grepWord( const char *cmd, const char *baseDir, const char *word, const unsigned int maxSize );
  result_t *getAllFileNames();
  result_t *findFileNames( const char *baseDir, const char *fileTypes, const unsigned int maxFileNameSize );
  result_t *getFirstShortenedFileNames();
  result_t *filterFileNames( const char *prompt );
  const char *getFullFileName( const unsigned int );
  void deleteFileNameResult( result_t * );
  void fillFileNames( const char **files, const unsigned int size, const unsigned int maxSize );
]]

local native = {}

native.cleanupResult = function(c_result)
  c_interface.deleteFileNameResult(c_result)
end

native.c_res_to_lua = function(c_res)
  if c_res == nil then
    return
  end

  local res = {}
  local viewSize = tonumber(c_res.viewFileNamesSize)
  for i = 1, viewSize do
    res[i] = ffi.string(c_res.viewFileNames[i - 1])
  end

  local positions = nil
  if c_res.positions ~= nil then
    positions = {}
    for i = 1, viewSize do
      local c_positionsItem = c_res.positions[i - 1]
      -- print(c_positionsItem)
      -- print('size:', c_positionsItem.size)
      -- print('data ptr:', c_positionsItem.data)
      local c_positionItemSize = tonumber(c_positionsItem.size)
      positions[i] = {}
      for pos = 1, c_positionItemSize do
        positions[i][pos] = c_positionsItem.data[pos - 1] + 1
      end
    end
  end

  local filteredSize = tonumber(c_res.filteredFileNamesSize)
  local allSize = tonumber(c_res.fileNamesSize)
  native.cleanupResult(c_res)

  return res, filteredSize, allSize, positions
end

native.fillFileNames = function(fileNames, maxFileNameSize)
  local arr = ffi.new('const char *[?]', #fileNames)
  for i = 1, #fileNames do
    arr[i - 1] = fileNames[i]
  end
  c_interface.fillFileNames(arr, #fileNames, maxFileNameSize)
end

native.findFileNames = function(cmd, fileTypes, maxFileNameSize)
  return native.c_res_to_lua(c_interface.findFileNames(cmd, fileTypes, maxFileNameSize))
end

native.grepWord = function(cmd, basedir, regexp, maxFileNameSize)
  return native.c_res_to_lua(c_interface.grepWord(cmd, basedir, regexp, maxFileNameSize))
end

native.filterFileNames = function(prompt)
  return native.c_res_to_lua(c_interface.filterFileNames(prompt))
end

native.getFullFileName = function(index)
  local ptr = c_interface.getFullFileName(index)
  if ptr == nil then
    vim.notify('fullFileName is nullptr', vim.log.levels.ERR)
    return
  end
  local res = ffi.string(ptr)
  return res
end

native.getAllFileNamesForOut = function()
  return native.c_res_to_lua(c_interface.getAllFileNames())
end

native.getFirstShortenedFileNames = function()
  return native.c_res_to_lua(c_interface.getFirstShortenedFileNames())
end

native.getAllFileNames = function(grepMode)
  local res = {}
  local fileResult = c_interface.getAllFileNames()
  local viewSize = tonumber(fileResult.viewFileNamesSize)
  for i = 1, viewSize do
    if grepMode then
      local file, lineStr, columnStr =
        ffi.string(fileResult.viewFileNames[i - 1]):match '([^:]+):(%d+):(%d+)'
      table.insert(res, {
        filename = file,
        lnum = tonumber(lineStr),
        col = tonumber(columnStr),
        text = 'found file',
      })
    else
      table.insert(res, {
        filename = ffi.string(fileResult.viewFileNames[i - 1]),
        lnum = 1,
        text = 'found file',
      })
    end
  end
  -- native.cleanupResult()
  return res
end
-- native.get_pos = function(input, pattern)
-- 	local pos = c_interface.fzs_get_positions(input, pattern)
-- 	if pos == nil then
-- 		return
-- 	end
--
-- 	local res = {}
-- 	for i = 1, tonumber(pos.size) do
-- 		res[i] = pos.data[i - 1] + 1
-- 	end
--
-- 	return res
-- end

return native
