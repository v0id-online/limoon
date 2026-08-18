-- Plugin: Word Count
-- Shows word / line count in the left statusbar segment.
-- Ctrl+Shift+W opens a detail dialog with selection stats.

local W = require('ui_widgets')
local commands = require('commands.registry')

-- Cache the word count and only recompute it when the buffer's text
-- actually changes. count() runs on every UPDATE_UI event (i.e. every
-- keystroke); buffer:get_text() copies the entire buffer, so recomputing
-- unconditionally made every keystroke pay an O(buffer size) cost.
local cached_words = 0
local cache_dirty  = true

local function recount_words()
  local text = buffer:get_text()
  local w = 0
  for _ in text:gmatch('%S+') do w = w + 1 end
  cached_words = w
  cache_dirty  = false
end

events.connect(events.BUFFER_AFTER_SWITCH, function() cache_dirty = true end)
events.connect(events.BUFFER_NEW, function() cache_dirty = true end)
events.connect(events.MODIFIED, function(_, mod_type)
  -- SC_MOD_INSERTTEXT = 1, SC_MOD_DELETETEXT = 2
  if mod_type and (mod_type & 3) ~= 0 then cache_dirty = true end
end)

local function count()
  if cache_dirty then recount_words() end
  return string.format('W:%d  L:%d', cached_words, buffer.line_count)
end

W.status_add('word_count', count, 20)

local function show_word_count()
  local text  = buffer:get_text()
  local chars = #text
  local lines = buffer.line_count
  local words = 0
  for _ in text:gmatch('%S+') do words = words + 1 end

  local sel      = buffer:get_sel_text()
  local sel_w, sel_c = 0, #sel
  for _ in sel:gmatch('%S+') do sel_w = sel_w + 1 end

  ui.dialogs.message{
    title = 'Document Statistics',
    text  = string.format(
      'Lines : %d\nWords : %d\nChars : %d\n\nSelection\n  Words : %d\n  Chars : %d',
      lines, words, chars, sel_w, sel_c),
    button1 = 'OK',
  }
end
keys['ctrl+shift+w'] = show_word_count

commands.register{
  id = 'edit.word_count',
  name = 'Word Count',
  description = 'Show document/selection word, line, and character counts',
  category = 'Edit',
  keybinding = 'ctrl+shift+w',
  action = show_word_count,
}

return {
  _meta = {
    name        = 'Word Count',
    description = 'Live word/line count in statusbar + Ctrl+Shift+W detail dialog',
    version     = '1.0',
    author      = 'built-in',
  }
}
