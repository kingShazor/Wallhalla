local M = {}

-- M.setup = function(_) end -- todo

-- to handle windows and bufs as conglomerate (useful for cleanup)
-- __index and __newindex ist set to M.state
M.state = {
  -- buffer und windows arrays [ input, output, preview ] track only 'static buffers'. preview-buf should be empty buffer only for presenting a non matching search;
  -- preview_buf saves the last shown buffer in M.wins[M.preview]
  bufs = {},
  wins = {},
  -- namespace for file extmark and selected line in file output
  ns_extmark = nil,
  ns_selectedLine = nil,
  -- mark for found files
  fileSizeMarkID = nil,
  -- marker for selected line
  selectedLine = 0,
  -- buf-ids:
  input = 1,
  output = 2,
  preview = 3,
  -- dont' filter files after initial file search - search pattern is ''
  init = false,
  -- prompt line will be moved when file open on empty set is triggered, - update the extmarks
  promptLine = 0,
  preview_buf = nil,

  -- function for initial filling of the out-buffer
  initOutFn = nil,
  -- function for live filtering via prompt
  liveFilteringFn = nil,

  -- last Search
  lastSearch = {
    prompt = nil,
    grep_cmd = nil,
    -- preview_mode uses also preview_mode with 3 windows
    preview_mode = false,
  },

  -- highlight
  positionHighlight = 'PositionHighlight',
  fakeCursor = 'FakeCursor',
  opt = {
    relative_height = 0.6,
    relative_width = 0.6,
    border_color = '#5a1010',
    position_color = '#b8743a',
    selection_bg = '#2a1646',
  },
}

setmetatable(M, { __index = M.state, __newindex = M.state })

M.run_find_bufs = function()
  local bufs = vim.api.nvim_list_bufs()
  local names = {}
  local i = 0
  local win = vim.api.nvim_get_current_win()
  local currentBuf = vim.api.nvim_win_get_buf(win)
  local cwd = vim.fn.expand(os.getenv 'PWD')
  for _, buf in ipairs(bufs) do
    if vim.api.nvim_get_option_value('buflisted', { buf = buf }) then
      local fileName = vim.api.nvim_buf_get_name(buf)
      if fileName ~= '' then
        i = i + 1
        local pos
        if buf == currentBuf then
          pos = vim.api.nvim_win_get_cursor(win)
        else
          pos = vim.api.nvim_buf_get_mark(buf, '"')
        end
        local relPath = vim.fs.relpath(cwd, fileName)
        fileName = relPath or fileName
        names[i] = fileName .. string.format(':%d:%d', pos[1], pos[2] + 1)
      end
    end
  end
  local native = require 'recipe-picker.native'
  native.fillFileNames(names, M.width_out)

  local res = native.getFirstShortenedFileNames()
  return res, #res, #res
end

M.run_find_files = function(opts)
  local native = require 'recipe-picker.native'
  return native.findFileNames(opts.cwd, opts.fileTypes, M.width_out)
end

M.run_grep = function(opts)
  local native = require 'recipe-picker.native'
  return native.grepWord(opts.cmd, opts.cwd, M.lastSearch.prompt, M.width_out)
end

M.run_filter_file_names = function()
  local native = require 'recipe-picker.native'
  return native.filterFileNames(M.lastSearch.prompt)
end

M.ui_cleanup = function()
  local inBuf = M.bufs[M.input]
  if vim.api.nvim_buf_is_valid(inBuf) then
    vim.api.nvim_buf_set_lines(inBuf, 0, -1, false, {})
    vim.bo[inBuf].modified = false
  end
  for _, win in ipairs(M.wins) do
    if vim.api.nvim_win_is_valid(win) then
      vim.api.nvim_win_close(win, true)
    end
  end

  for _, buf in ipairs(M.bufs) do
    if vim.api.nvim_buf_is_valid(buf) then
      vim.schedule(function()
        if vim.api.nvim_buf_is_valid(buf) then
          vim.api.nvim_buf_delete(buf, { force = true })
        end
      end)
    end
  end

  if M.preview_buf then
    vim.api.nvim_buf_clear_namespace(M.preview_buf, M.ns_selectedLine, 0, -1)
    vim.api.nvim_buf_delete(M.preview_buf, { unload = true })
    M.preview_buf = nil
  end
end

M.buf_real_line_count = function(buf)
  local bufLines = vim.api.nvim_buf_line_count(buf)
  if bufLines > 1 then
    return bufLines
  end
  local lines = vim.api.nvim_buf_get_lines(buf, 0, -1, false)
  if lines[1] == '' then
    return 0
  end
  return 1
end

M.state_reset = function()
  M.selectedLine = 0
end

-- reset all states
M.run_exit = function()
  M.ui_cleanup()
  M.state_reset()
end

M.ui_highlight_output = function()
  if not M.ns_selectedLine then
    M.ns_selectedLine = vim.api.nvim_create_namespace 'file search selected line'
  else
    vim.api.nvim_buf_clear_namespace(M.bufs[M.output], M.ns_selectedLine, 0, -1)
  end

  local buf_out = M.bufs[M.output]
  local bufLine = M.buf_real_line_count(buf_out)
  if bufLine > 0 then
    vim.api.nvim_win_set_cursor(M.wins[M.output], { M.selectedLine + 1, 0 })
    vim.hl.range(
      M.bufs[M.output],
      M.ns_selectedLine,
      'PickerSelection',
      { M.selectedLine, 0 },
      { M.selectedLine, -1 }
    )
  end

  -- highlight the preview-window
  vim.api.nvim_set_current_win(M.wins[M.input])
  if not M.lastSearch.preview_mode then
    return
  end
  if bufLine <= 0 then
    vim.api.nvim_win_set_buf(M.wins[M.preview], M.bufs[M.preview])
    return
  end
  local native = require 'recipe-picker.native'
  local str = native.getFullFileName(M.selectedLine)
  if str == nil then
    return --already reported
  end
  local file, lineStr, columnStr, greppedSizeInByte =
    str:match '([^:]+):(%d+):(%d+):?(%d*)'
  if not file then
    file = str
  end

  if greppedSizeInByte == '' then
    greppedSizeInByte = nil
  end

  if file then
    local buf = vim.fn.bufadd(file)
    local ok, err = pcall(vim.fn.bufload, buf)
    if not ok then
      if err:match 'E325' then
        vim.notify(
          string.format('W325 Found swap file for %s', file),
          vim.log.levels.WARN
        )
      else
        error(err)
      end
    end
    vim.api.nvim_buf_clear_namespace(buf, M.ns_selectedLine, 0, -1)
    vim.api.nvim_win_set_buf(M.wins[M.preview], buf)
    if M.preview_buf ~= buf then
      if M.preview_buf then
        vim.api.nvim_buf_clear_namespace(M.preview_buf, M.ns_selectedLine, 0, -1)
        vim.api.nvim_buf_delete(M.preview_buf, { unload = true })
      end
      M.preview_buf = buf
    end
    if lineStr and columnStr then
      local line = tonumber(lineStr)
      local column = tonumber(columnStr)
      vim.api.nvim_win_set_cursor(M.wins[M.preview], { line, column - 1 })
      -- need to reset hl, because opening a buffer will change highligthing
      vim.wo[M.wins[M.preview]].winhl = 'Normal:Normal,FloatBorder:BorderPicker'
      vim.hl.range(
        buf,
        M.ns_selectedLine,
        'PickerSelection',
        { line - 1, 0 },
        { line - 1, -1 }
      )
      if greppedSizeInByte ~= nil then
        local endHighlight = greppedSizeInByte + column - 1
        vim.hl.range(
          buf,
          M.ns_selectedLine,
          M.positionHighlight,
          { line - 1, column - 1 },
          { line - 1, endHighlight }
        )
      else -- set fake cursor
        vim.hl.range(
          buf,
          M.ns_selectedLine,
          M.fakeCursor,
          { line - 1, column - 1 },
          { line - 1, column },
          { priority = vim.hl.priorities.user + 1 }
        )
      end
    end
  else
    vim.notify('file could not be opened', vim.log.levels.WARN)
  end
end

M.ui_move_up = function()
  local buf_out = M.bufs[M.output]
  local bufLines = M.buf_real_line_count(buf_out)
  if bufLines == 0 then
    return
  end

  if M.selectedLine == 0 then
    M.selectedLine = bufLines - 1
  else
    M.selectedLine = M.selectedLine - 1
  end

  M.ui_highlight_output()
end

M.ui_move_down = function()
  local buf_out = M.bufs[M.output]
  local bufLines = M.buf_real_line_count(buf_out)
  if bufLines == 0 then
    return
  end

  M.selectedLine = M.selectedLine + 1
  if M.selectedLine >= bufLines then
    M.selectedLine = 0
  end

  M.ui_highlight_output()
end

M.picker_add_keymap = function()
  for _, buf in ipairs(M.bufs) do
    if vim.api.nvim_buf_is_valid(buf) then
      vim.keymap.set('n', 'q', M.run_exit, { buffer = buf, desc = 'close picker' })
      vim.keymap.set('n', '<Esc>', M.run_exit, { buffer = buf, desc = 'close picker' })
      vim.keymap.set(
        { 'n', 'i' },
        '<Up>',
        M.ui_move_up,
        { buffer = buf, desc = 'move selected line up' }
      )
      vim.keymap.set(
        { 'n', 'i' },
        '<Down>',
        M.ui_move_down,
        { buffer = buf, desc = 'move selected line down' }
      )
      vim.keymap.set(
        { 'n', 'i' },
        '<C-q>',
        M.picker_send_to_quickfix,
        { buffer = buf, desc = 'fill quick fix list with found file name' }
      )
    end
  end
end

M.run_open_file = function(native)
  --cleanup, so the file can be opened with relative linenumbers
  M.ui_cleanup()
  if M.lastSearch.preview_mode then
    local str = native.getFullFileName(M.selectedLine)
    local file, lineStr, columnStr = str:match '([^:]+):(%d+):(%d+)'
    vim.api.nvim_command('edit ' .. file)
    vim.api.nvim_buf_clear_namespace(0, M.ns_selectedLine, 0, -1)
    vim.api.nvim_win_set_cursor(0, { tonumber(lineStr), tonumber(columnStr) })
  else
    vim.api.nvim_command(
      'edit ' .. M.opt.cwd .. '/' .. native.getFullFileName(M.selectedLine)
    )
  end
  M.state_reset()
end

-- created autocmd for TextChangedI for input field:
-- execute a file name lookup
-- update buf_out
-- also update: found files count, and highlighting
M.picker_create_autocmds = function()
  vim.api.nvim_create_autocmd('TextChangedI', {
    buffer = M.bufs[M.input],
    callback = function()
      if not M.init then -- ignore first TextChangedI call after initial findFiles
        M.init = true
        return
      end
      local buf_in = M.bufs[M.input]
      local buf_out = M.bufs[M.output]
      -- first chars are '> '
      M.lastSearch.prompt = string.sub(vim.api.nvim_get_current_line(), 3)
      local lines, filteredSize, allSize, positionsItem = M.liveFilteringFn()
      vim.api.nvim_buf_set_lines(buf_out, 0, -1, false, lines)

      M.selectedLine = 0
      M.ui_highlight_output()

      if positionsItem then
        if lines and #lines > 0 then
          for line, _ in ipairs(lines) do
            local indices = positionsItem[line]
            for _, col in ipairs(indices) do
              vim.hl.range(
                buf_out,
                M.ns_selectedLine,
                M.positionHighlight,
                { line - 1, col - 1 },
                { line - 1, col }
              )
            end
          end
        end
      end

      vim.api.nvim_buf_set_extmark(buf_in, M.ns_extmark, M.promptLine, 0, {
        id = M.fileSizeMarkID,
        virt_text = { { string.format('%s/%s', filteredSize, allSize), 'Comment' } },
        virt_text_pos = 'right_align',
      })
    end,
  })

  vim.api.nvim_create_autocmd('BufHidden', {
    buffer = M.bufs[M.input],
    callback = function()
      M.ui_cleanup()
    end,
  })

  vim.api.nvim_create_autocmd('BufHidden', {
    buffer = M.bufs[M.output],
    callback = function()
      M.ui_cleanup()
    end,
  })
end

M.picker_send_to_quickfix = function()
  local native = require 'recipe-picker.native'
  M.ui_cleanup()
  vim.fn.setqflist(native.getAllFileNames(M.lastSearch.preview_mode), 'r')
  if #vim.fn.getqflist() > 0 then
    vim.cmd 'copen'
  end
end

-- todo get recipe
M.picker_start = function(opts, preview_mode)
  local native = require 'recipe-picker.native'
  local buf_out = vim.api.nvim_create_buf(false, true)
  M.opt = vim.tbl_extend('force', M.opt or {}, opts or {})
  M.lastSearch.preview_mode = preview_mode

  M.init = M.lastSearch.prompt ~= nil

  M.promptLine = 0
  local width_all = math.floor(vim.o.columns * M.opt.relative_width)
  local height_all = math.floor(vim.o.lines * M.opt.relative_height)
  local row = math.floor((vim.o.lines - height_all) / 2)
  local col = math.floor((vim.o.columns - width_all) / 2)

  if preview_mode then
    row = 1
    col = 0
    height_all = vim.o.lines - 5
  end

  local height_input = 3

  M.width_out = width_all
  local height_out = height_all - height_input
  local rows_out = row + height_input

  local filteredSize
  local allSize
  -- if not preview_mode then
  if M.initOutFn then
    local lines, fSize, aSize = M.initOutFn()
    vim.api.nvim_buf_set_lines(buf_out, 0, -1, false, lines)
    filteredSize = fSize
    allSize = aSize
  end
  -- end

  local buf_in = vim.api.nvim_create_buf(false, true)

  local opt = {
    relative = 'editor',
    width = M.width_out,
    height = 1,
    title = ' File Search ',
    title_pos = 'center',
    row = row,
    col = col,
    border = 'rounded',
    style = 'minimal',
  }

  local win_in = vim.api.nvim_open_win(buf_in, true, opt)
  opt.height = height_out
  opt.row = rows_out
  opt.title = ' Results '

  local win_out = vim.api.nvim_open_win(buf_out, true, opt)
  local buf_preview = nil
  local win_preview = nil
  if preview_mode then
    opt.row = 1
    opt.col = opt.width + 2
    opt.width = vim.o.columns - opt.col - 2
    opt.title = ' Preview '
    opt.height = opt.height + 3
    buf_preview = vim.api.nvim_create_buf(false, true)
    win_preview = vim.api.nvim_open_win(buf_preview, true, opt)
  end

  vim.api.nvim_set_hl(0, 'BorderPicker', { fg = M.opt.border_color, bg = 'NONE' })
  vim.api.nvim_set_hl(0, M.positionHighlight, { fg = M.opt.position_color, bold = true })
  vim.api.nvim_set_hl(
    0,
    M.fakeCursor,
    { bg = M.opt.position_color, fg = M.opt.selection_bg }
  )
  vim.api.nvim_set_hl(0, 'PickerSelection', { bg = M.opt.selection_bg })

  local hl = 'Normal:Normal,FloatBorder:BorderPicker'
  vim.wo[win_out].winhl = hl
  vim.wo[win_in].winhl = hl

  if win_preview then
    vim.wo[win_preview].winhl = hl
  end

  M.ns_extmark = vim.api.nvim_create_namespace ''
  vim.bo[buf_in].buftype = 'prompt'
  vim.bo[buf_in].modifiable = true
  -- vim.bo[buf_in].bufhidden = 'wipe'
  vim.fn.prompt_setprompt(buf_in, '> ')
  vim.fn.prompt_setcallback(buf_in, function(_)
    local fileNamesSize = M.buf_real_line_count(buf_out)
    if fileNamesSize < 1 then
      vim.notify(string.format 'No files selected!', vim.log.levels.WARN)
      M.promptLine = M.promptLine + 1
      return
    end
    M.run_open_file(native)
  end)
  M.fileSizeMarkID = vim.api.nvim_buf_set_extmark(buf_in, M.ns_extmark, M.promptLine, 0, {
    virt_text = { { string.format('%s/%s', filteredSize, allSize), 'Comment' } },
    virt_text_pos = 'right_align',
  })
  vim.api.nvim_set_current_win(win_in)
  vim.cmd.startinsert()
  if M.lastSearch.prompt then
    vim.api.nvim_feedkeys(
      vim.api.nvim_replace_termcodes(M.lastSearch.prompt, true, false, true),
      'i',
      true
    )
  end

  M.bufs = { buf_in, buf_out, buf_preview }
  M.wins = { win_in, win_out, win_preview }
  M.picker_add_keymap()
  M.picker_create_autocmds()
  M.ui_highlight_output()
  -- vim.notify("hello world!", vim.log.levels.info)
end

M.ensure_cwd = function(opts)
  if opts == nil then
    opts = {}
  end
  opts.cwd = opts.cwd or vim.fn.expand(os.getenv 'PWD')
  return opts
end

M.search_file = function(opts)
  M.lastSearch.prompt = nil
  opts = M.ensure_cwd(opts)

  M.initOutFn = function()
    return M.run_find_files(opts)
  end
  M.liveFilteringFn = M.run_filter_file_names
  M.picker_start(opts, false)
end

M.search_regex = function(prompt)
  M.lastSearch.prompt = prompt
  M.initOutFn = nil

  local opts = {}
  opts.cmd = "rg -o '{}' --column 2>/dev/null | head -n 20000"
  opts.cwd = vim.fn.expand(os.getenv 'PWD')

  M.liveFilteringFn = function()
    return M.run_grep(opts)
  end
  M.picker_start({ relative_width = 0.4 }, true)
end

M.search_text = function(prompt)
  M.lastSearch.prompt = prompt
  M.initOutFn = nil

  local opts = {}
  opts.cmd = "rg -o -F '{}' --column 2>/dev/null | head -n 20000"
  opts.cwd = vim.fn.expand(os.getenv 'PWD')

  M.liveFilteringFn = function()
    return M.run_grep(opts)
  end
  M.picker_start({ relative_width = 0.4 }, true)
end

M.search_resume = function()
  if M.lastSearch then
    M.picker_start({}, M.lastSearch.preview_mode)
  end
end

M.search_buf = function()
  M.lastSearch.prompt = nil
  M.initOutFn = M.run_find_bufs
  M.liveFilteringFn = M.run_filter_file_names
  M.picker_start({ relative_width = 0.4 }, true)
end

return M
