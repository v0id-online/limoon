-- aliases.lua — short command-entry globals for Li Moon
-- Usage: press Ctrl+; and type any alias below.
-- Call  aliases()  to display this list inside the editor.
--
-- Convention: one-letter or short names that don't shadow Lua builtins.

-- ── Themes ────────────────────────────────────────────────────────────────
-- theme "dracula"      → apply theme by name
-- themes()             → interactive picker
-- setcolor("green")    → set UI color scheme (green/blue/bw)
if limoon.themes then
  theme  = function(n) limoon.themes.set(n)    end
  themes = function()  limoon.themes.select()  end
end

setcolor = function(c)
  local colors = {green = 'green', blue = 'blue', bw = 'bw', black = 'bw', white = 'bw'}
  local color = colors[c and c:lower() or '']
  if color then
    -- Save to file for persistence
    local f = io.open(_USERHOME .. '/theme_color', 'w')
    if f then f:write(color) f:close() end
    ui.statusbar_text = 'UI color set to: ' .. color .. ' (restart to apply)'
  else
    ui.statusbar_text = 'Usage: setcolor("green") or setcolor("blue") or setcolor("bw")'
  end
end

-- ── Files / Buffers ────────────────────────────────────────────────────────
-- e "path"   → open file (Edit)
-- save()     → save current buffer
-- saveas "p" → save as new path
-- bclose()   → close current buffer
-- reload()   → reload from disk
e      = function(p) if p then io.open_file(p) else ui.statusbar_text = 'e(path): missing path' end end
save   = function()  buffer:save() end
saveas = function(p) buffer:save_as(p) end
bclose = function()  buffer:close() end
reload = function()  buffer:reload() end

-- ── Buffer navigation ──────────────────────────────────────────────────────
-- bnext() / bprev()   → cycle buffers
bnext = function() view:goto_buffer(1)  end
bprev = function() view:goto_buffer(-1) end

-- ── View/Split ─────────────────────────────────────────────────────────────
-- sp()       → split view vertical (Ctrl+>)
-- uns()      → unsplit view (Ctrl+<)
-- unsplitall() → close all splits
sp       = function() view:split(true) end
uns      = function() view:unsplit() end
unsplitall = function() while view:unsplit() do end end

-- ── Editor navigation ──────────────────────────────────────────────────────
-- ln(n)   → go to line n (1-based)
ln = function(n) limoon.editing.goto_line(n) end

-- ── Syntax / Lexer ─────────────────────────────────────────────────────────
-- lex "lua"   → set syntax highlight language
lex = function(n) buffer:set_lexer(n) end

-- ── Display toggles ────────────────────────────────────────────────────────
-- wrap()       → toggle line wrap
-- tabs(n)      → use hard tabs, width n (default 4)
-- spaces(n)    → use soft spaces, width n (default 2)
-- zoom(n)      → n>0 zoom in, n<0 zoom out, n==0 reset (default 0)
wrap = function()
  view.wrap_mode = view.wrap_mode == view.WRAP_NONE
    and view.WRAP_WHITESPACE or view.WRAP_NONE
  ui.statusbar_text = 'Wrap: ' .. (view.wrap_mode ~= view.WRAP_NONE and 'on' or 'off')
end

tabs = function(n)
  buffer.use_tabs  = true
  buffer.tab_width = n or 4
  ui.statusbar_text = 'Tabs: width ' .. buffer.tab_width
end

spaces = function(n)
  buffer.use_tabs  = false
  buffer.tab_width = n or 2
  ui.statusbar_text = 'Spaces: width ' .. buffer.tab_width
end

zoom = function(n)
  n = n or 0
  if     n > 0 then buffer:zoom_in()
  elseif n < 0 then buffer:zoom_out()
  else              view.zoom = 0
  end
end

-- ── Plugins ────────────────────────────────────────────────────────────────
if limoon.plugins then
  plugins = function() limoon.plugins.show() end
end

-- ── Run ────────────────────────────────────────────────────────────────────
-- run()   → run current file via limoon.run
run = function() limoon.run.run() end

-- ── Help ───────────────────────────────────────────────────────────────────
local _list = {
  -- {alias,            description}
  {'theme "name"',    'apply theme  e.g. theme "dracula"'},
  {'themes()',        'interactive theme picker'},
  {'e "path"',        'open file'},
  {'save()',          'save current buffer'},
  {'saveas "path"',   'save buffer as new file'},
  {'bclose()',        'close current buffer'},
  {'reload()',        'reload file from disk'},
  {'bnext()',         'go to next buffer'},
  {'bprev()',         'go to prev buffer'},
  {'sp()',            'split view vertical (Ctrl+>)'},
  {'uns()',           'unsplit view (Ctrl+<)'},
  {'unsplitall()',    'close all splits'},
  {'ln(n)',           'go to line n'},
  {'lex "name"',      'set syntax  e.g. lex "lua"'},
  {'wrap()',          'toggle line wrap on/off'},
  {'tabs(n)',         'use hard tabs, width n  (default 4)'},
  {'spaces(n)',       'use soft spaces, width n  (default 2)'},
  {'zoom(n)',         'zoom: 1=in  -1=out  0=reset'},
  {'run()',           'run current file'},
  {'plugins()',       'show loaded plugins'},
  {'setcolor("name")','set UI color (green/blue/bw)'},
  {'aliases()',       'show this list'},
}

aliases = function()
  local W = require('ui_widgets')
  local buf = W.output_buffer('[Aliases]')
  local lines = {}
  lines[#lines + 1] = 'Command Entry Aliases  (open with Ctrl+;)\n'
  lines[#lines + 1] = string.rep('─', 48) .. '\n'
  for _, a in ipairs(_list) do
    -- skip theme/plugins aliases when those modules aren't loaded
    if (a[1]:find('^theme') and not limoon.themes) or
       (a[1] == 'plugins()' and not limoon.plugins) then
      -- skip
    else
      lines[#lines + 1] = string.format('  %-20s %s\n', a[1], a[2])
    end
  end
  W.write(buf, table.concat(lines), true)
end
