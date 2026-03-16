-- Plugin: Help
-- Opens a read-only quick-reference buffer on Ctrl+H.
-- A persistent hint is shown at the right of the status bar.

local W = require('ui_widgets')

local HELP_TYPE = '[Help]'

-- ── Content ───────────────────────────────────────────────────────────────

-- Theme rows formatted to fit ~70 cols (4-space indent)
local THEME_ROWS = {
  'ayu-dark  ayu-mirage  catppuccin-latte  catppuccin-mocha',
  'dracula  everforest-dark  github-dark  github-light',
  'gruvbox-dark  gruvbox-light  kanagawa  material-dark',
  'monokai  nord  one-dark  rose-pine  solarized-dark',
  'solarized-light  tokyo-night  tomorrow-night',
}

local function section(title)
  return '\n── ' .. title .. ' ' .. string.rep('─', 66 - #title) .. '\n'
end

local CONTENT = table.concat({

  '╭' .. string.rep('─', 70) .. '╮\n',
  '│  TEXTADEPT  ·  Quick Reference' .. string.rep(' ', 22) .. 'Ctrl+H to close  │\n',
  '╰' .. string.rep('─', 70) .. '╯\n',

  section('FILES & BUFFERS'),
  '  Ctrl+O             Open file\n',
  '  Ctrl+S             Save                  Ctrl+Shift+S   Save As\n',
  '  Ctrl+W             Close buffer          Ctrl+N         New buffer\n',
  '  Ctrl+Tab           Next buffer           Ctrl+Shift+Tab Prev buffer\n',
  '  Ctrl+Q             Quit\n',

  section('EDITING'),
  '  Ctrl+Z / Ctrl+Y    Undo / Redo\n',
  '  Ctrl+X / C / V     Cut / Copy / Paste    Ctrl+A         Select all\n',
  '  Ctrl+D             Duplicate line        Ctrl+/         Toggle comment\n',
  '  Tab / Shift+Tab    Indent / Unindent\n',

  section('FIND & REPLACE'),
  '  Ctrl+F             Open / close find bar\n',
  '  Enter              Find next             Shift+Enter    Find prev\n',
  '  Tab / Shift+Tab    Cycle buttons: entry→Prev→Next→HiAll→Close\n',
  '  Esc / Ctrl+F       Close find bar\n',

  section('BOOKMARKS'),
  '  Ctrl+F2            Toggle bookmark\n',
  '  F2                 Next bookmark         Shift+F2       Prev bookmark\n',

  section('CODE FOLDING'),
  '  F6                 Toggle fold at current line\n',
  '  (Click fold margin +/- to collapse/expand with mouse)\n',
  '  FOLD_ENABLED=true  Flag in init.lua to enable/disable folding\n',

  section('THEMES'),
  '  In command entry (Ctrl+;):\n',
  '    theme "name"   — apply a theme directly\n',
  '    themes()       — interactive picker\n',
  '\n',
  '  Available themes:\n',
  '    ' .. THEME_ROWS[1] .. '\n',
  '    ' .. THEME_ROWS[2] .. '\n',
  '    ' .. THEME_ROWS[3] .. '\n',
  '    ' .. THEME_ROWS[4] .. '\n',
  '    ' .. THEME_ROWS[5] .. '\n',

  section('PLUGINS'),
  '  Ctrl+H             Help (this buffer — toggle)\n',
  '  Ctrl+Shift+N       Scratch Pad (persistent notes, auto-saves)\n',
  '  Ctrl+Shift+G       Git Status panel\n',
  '  Ctrl+Shift+W       Word Count dialog\n',
  '  Ctrl+Shift+P       Plugin list\n',

  section('COMMAND ENTRY  (Ctrl+E)'),
  '  Tab                Cycle completions     (statusbar shows options)\n',
  '\n',
  '  ── Aliases (type aliases() for the full list) ──\n',
  '  theme "name"       Apply theme           themes()       Picker\n',
  '  e "path"           Open file             save()         Save\n',
  '  saveas "path"      Save As               bclose()       Close buffer\n',
  '  reload()           Reload from disk      run()          Run file\n',
  '  bnext() / bprev()  Next / prev buffer\n',
  '  ln(n)              Go to line n          lex "name"     Set syntax\n',
  '  wrap()             Toggle line wrap      zoom(n)        ±1 / 0=reset\n',
  '  tabs(n)            Hard tabs width n     spaces(n)      Spaces width n\n',
  '  plugins()          Plugin list           aliases()      All aliases\n',
  '\n',
  '  ── Direct Lua (buffer/view/ui/textadept shortcuts) ──\n',
  '  buffer:save()      Save current buffer\n',
  '  buffer:close()     Close current buffer\n',
  '  view:split()       Split view horizontally\n',
  '  io.open_file("p")  Open file at path p\n',
  '  print(expr)        Print value to output buffer\n',
  '  os.execute("cmd")  Run a shell command\n',

  '\n' .. string.rep('─', 70) .. '\n',
  '  Press Ctrl+H to close this buffer.\n',

}, '')

-- ── Open / toggle ─────────────────────────────────────────────────────────

local function open_help()
  -- Toggle off: close and remove from buffer/tab list entirely.
  if buffer._type == HELP_TYPE then
    buffer:set_save_point()  -- clear modify flag to skip save dialog
    buffer:close()
    return
  end
  -- If help buffer exists elsewhere in the list, just switch to it.
  for _, b in ipairs(_BUFFERS) do
    if b._type == HELP_TYPE then
      view:goto_buffer(b)
      return
    end
  end
  -- Create a new read-only help buffer.
  local buf = buffer.new()
  buf._type     = HELP_TYPE
  buf.read_only = false
  buf:append_text(CONTENT)
  buf:set_lexer('text')
  buf:goto_pos(0)     -- position cursor BEFORE locking (avoids modify flag side-effects)
  buf:set_save_point()  -- clear the modify flag set by append_text
  buf.read_only = true
end

keys['ctrl+h'] = open_help

-- ── Status bar hint ───────────────────────────────────────────────────────
-- Priority 90 → appears at the far right, after word count and git status.

W.status_add('help_hint', function() return '? Ctrl+H' end, 90)

-- ── Plugin metadata ───────────────────────────────────────────────────────

return {
  _meta = {
    name        = 'Help',
    description = 'Quick-reference buffer (Ctrl+H), hint in statusbar',
    version     = '1.0',
    author      = 'built-in',
  }
}
