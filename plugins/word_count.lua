-- Plugin: Word Count
-- Shows word / line count in the left statusbar segment.
-- Ctrl+Shift+W opens a detail dialog with selection stats.

local W = require('ui_widgets')
local commands = require('commands.registry')

local function count()
  local text  = buffer:get_text()
  local words = 0
  for _ in text:gmatch('%S+') do words = words + 1 end
  return string.format('W:%d  L:%d', words, buffer.line_count)
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
