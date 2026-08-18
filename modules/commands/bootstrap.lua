-- bootstrap.lua — Registers Li Moon's existing menu commands into the
-- command registry (modules/commands/registry.lua), so the Ctrl+P
-- palette (limoon.menu.select_command, in modules/limoon/menu.lua) has
-- full parity with the menubar without hand-duplicating ~150 entries.
--
-- Runs once, after events.INITIALIZED, when the real menubar table
-- exists (see modules/limoon/menu.lua's own events.INITIALIZED hook).
--
-- This module only reads the menubar and calls commands.register(); it
-- does not modify modules/limoon/menu.lua's tables in any way.

local commands = require('commands.registry')

-- Menu section title -> registry category. The registry's fixed category
-- set (File/Edit/View/Buffer/Window/Git/LSP/DAP) has no slot for every
-- section Li Moon's menubar ships, so sections without a clean match are
-- folded into the closest fit rather than inventing new categories:
--   Search -> Edit   (find/replace is a text-editing operation)
--   Tools  -> Edit   (command entry, run/build, bookmarks, macros, and
--                     snippets are editing-productivity utilities; none
--                     of them is really "Window", "Git", "LSP", or "DAP")
--   Help   -> View   (informational/display commands)
-- This mapping is a deliberate, reviewed judgment call — not a guess per
-- item. See docs/commands.md for the same note.
local SECTION_CATEGORY = {
  File = 'File', Edit = 'Edit', Search = 'Edit', Tools = 'Edit',
  Buffer = 'Buffer', View = 'View', Help = 'View', Window = 'Window',
}

-- accelerator_letter is left nil for every auto-registered command: the
-- default menubar never uses '_'/'&' accelerator markup in its labels
-- (see modules/limoon/menu.lua's own gsub('[_&]([^_&])', '%1'), which is
-- defensive/no-op against this menubar), so there is nothing to derive
-- one from. Flagged once here rather than per-command.

local function slug(s)
  return (s:lower():gsub('[^%w]+', '_'):gsub('^_+', ''):gsub('_+$', ''))
end

--- Recursively registers leaf items of a menu (sub)table.
-- @param menu Raw menu table (as stored in modules/limoon/menu.lua's
--   default_menubar — NOT the proxy `limoon.menu.menubar` returns).
-- @param path Dotted id prefix built from ancestor titles.
-- @param category Registry category to assign every leaf found here.
-- @param key_shortcuts function -> key-sequence-string reverse lookup.
-- @param seen_ids id -> true, used to disambiguate id collisions
--   (e.g. two "Save As" items under different sections).
local function walk(menu, path, category, key_shortcuts, seen_ids)
  for _, item in ipairs(menu) do
    if item.title then
      walk(item, path .. '.' .. slug(item.title), category, key_shortcuts, seen_ids)
    elseif item[1] and item[1] ~= '' and type(item[2]) == 'function' then
      local label = item[1]:gsub('[_&]([^_&])', '%1')
      local id = path .. '.' .. slug(label)
      if seen_ids[id] then
        local n = 2
        while seen_ids[id .. '_' .. n] do n = n + 1 end
        id = id .. '_' .. n
      end
      seen_ids[id] = true
      commands.register{
        id = id,
        name = label,
        category = category,
        keybinding = key_shortcuts[item[2]],
        action = item[2],
      }
    end
    -- Items whose item[2] is a table (menu items with bound arguments,
    -- e.g. {fn, arg}) are intentionally skipped: none exist in the
    -- current default_menubar, and registering a mis-called action would
    -- be worse than omitting it. Revisit if that pattern is introduced.
  end
end

events.connect(events.INITIALIZED, function()
  local key_shortcuts = {}
  for k, f in pairs(keys) do key_shortcuts[f] = k end

  local raw_menubar = getmetatable(limoon.menu.menubar).menu
  local seen_ids = {}
  for _, section in ipairs(raw_menubar) do
    local category = section.title and SECTION_CATEGORY[section.title]
    if category then walk(section, slug(section.title), category, key_shortcuts, seen_ids) end
  end

  -- modules/limoon/menu.lua's M.select_command() special-cases two
  -- commands that live outside the menu table entirely (theme picker,
  -- UI color picker). Register them too so the palette's coverage
  -- doesn't silently exclude them.
  commands.register{
    id = 'view.select_theme',
    name = 'Select Theme',
    description = 'Interactively pick a syntax theme',
    category = 'View',
    action = function() if limoon.themes then limoon.themes.select() end end,
  }
  commands.register{
    id = 'view.set_ui_color',
    name = 'Set UI Color',
    description = 'Choose the terminal UI accent color (green/blue/bw); restart to apply',
    category = 'View',
    action = function()
      local color = ui.dialogs.input{
        title = 'Set UI Color', text = 'Enter color (green, blue, or bw):',
      }
      if not color then return end
      local colors = {green = 'green', blue = 'blue', bw = 'bw', black = 'bw', white = 'bw'}
      local c = colors[color:lower()]
      if not c then ui.statusbar_text = 'Invalid color. Use: green, blue, or bw'; return end
      local f = io.open(_USERHOME .. '/theme_color', 'w')
      if f then f:write(c); f:close() end
      ui.statusbar_text = 'UI color: ' .. c .. ' (restart to apply)'
    end,
  }
end)

return {}
