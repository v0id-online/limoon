-- Theme manager for Textadept.
-- Usage:
--   textadept.themes.set('dracula')   -- apply a theme
--   textadept.themes.select()         -- interactive picker

local M = {}

-- All available themes (built-in + community)
M.available = {
  -- Built-in
  'dark', 'light', 'term',
  -- High Contrast
  'highcontrast', 'phosphor',
  -- Argonaut
  'argonaut',
  -- Community
  'monokai', 'dracula', 'one-dark', 'nord',
  'solarized-dark', 'solarized-light',
  'gruvbox-dark', 'gruvbox-light',
  'material-dark', 'tokyo-night',
  'catppuccin-mocha', 'catppuccin-latte',
  'github-dark', 'github-light',
  'ayu-dark', 'ayu-mirage',
  'tomorrow-night', 'everforest-dark',
  'rose-pine', 'kanagawa',
}

local pref_file = _USERHOME .. '/theme'

-- Returns the last saved theme name, or nil.
function M.get_saved()
  local f = io.open(pref_file, 'r')
  if not f then return nil end
  local name = f:read('*l')
  f:close()
  return name and name:match('^%s*(.-)%s*$') ~= '' and name:match('^%s*(.-)%s*$') or nil
end

-- Saves the theme name to the preference file.
function M.save(name)
  local f = io.open(pref_file, 'w')
  if f then f:write(name .. '\n'); f:close() end
end

--- Background transparency percentage (0 = opaque, 1-99 = blend, 100 = transparent).
-- Set this before or after calling set(); it takes effect on the next render.
M.bg_alpha = 0

-- Applies `name` to all open views and the command entry, then saves the choice.
function M.set(name)
  for _, v in ipairs(_VIEWS) do v:set_theme(name) end
  ui.command_entry:set_theme(name)
  M.current = name
  M.save(name)
  if CURSES then ui.bg_alpha = M.bg_alpha end
  ui.statusbar_text = 'Theme: ' .. name
end

-- Shows an interactive list dialog to pick a theme.
function M.select()
  local items = {}
  for _, name in ipairs(M.available) do
    items[#items + 1] = (name == M.current and '* ' or '  ') .. name
  end
  local choice = ui.dialogs.list{
    title   = 'Select Theme',
    columns = {'Theme'},
    items   = items,
  }
  if choice then
    local name = M.available[choice]
    if name then M.set(name) end
  end
end

-- Initialize: apply saved preference or default.
-- In CURSES mode, only use terminal-compatible themes.
local saved = M.get_saved()
if CURSES and saved and saved ~= 'term' and saved ~= 'dark' and saved ~= 'light'
    and saved ~= 'highcontrast' and saved ~= 'phosphor' and saved ~= 'argonaut' then
  saved = nil -- ignore incompatible saved theme
end
M.current = saved or 'dark'

-- On startup, send the initial bg_alpha so WezTerm matches from launch.
-- Register unconditionally: init.lua may set M.bg_alpha after this module loads.
if CURSES then
  events.connect(events.INITIALIZED, function()
    if M.bg_alpha > 0 then ui.bg_alpha = M.bg_alpha end
  end)
end

return M
