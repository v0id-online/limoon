-- registry.lua — Central command registry for Li Moon.
--
-- A command is any named, invokable action a user can trigger — the same
-- set of things that show up in a menu or a command palette. This module
-- is the single source of truth for the Ctrl+P palette (see
-- modules/limoon/menu.lua's M.select_command, which reads from
-- M.list() grouped by category).
--
-- Usage:
--   local commands = require('commands.registry')
--   commands.register{
--     id = 'file.save',
--     name = 'Save',
--     description = 'Save current buffer to disk',
--     category = 'File',
--     keybinding = 'Ctrl+S',
--     accelerator_letter = 'S',
--     action = function() buffer:save() end,
--   }

local M = {}

--- Fixed category set, in display order. Every command must use one of
-- these — there is no mechanism to add new categories at runtime.
M.CATEGORIES = {'File', 'Edit', 'View', 'Buffer', 'Window', 'Git', 'LSP', 'DAP'}

local _valid_category = {}
for _, c in ipairs(M.CATEGORIES) do _valid_category[c] = true end

M._commands = {}  -- [id] = command table
M._seq = 0  -- monotonic registration counter, used to keep M.list() stable

--- Register a command.
-- @param cmd table with fields:
--   id (string, required, unique)
--   name (string, required)
--   description (string, optional)
--   category (string, required, must be one of M.CATEGORIES)
--   keybinding (string, optional, display-only — does not itself bind a key)
--   accelerator_letter (string, optional, single letter for palette UX)
--   action (function, required)
function M.register(cmd)
  assert(type(cmd) == 'table', 'commands.registry.register: cmd must be a table')
  assert(type(cmd.id) == 'string' and cmd.id ~= '', 'commands.registry.register: cmd.id required')
  assert(type(cmd.name) == 'string' and cmd.name ~= '', 'commands.registry.register: cmd.name required')
  assert(type(cmd.action) == 'function', 'commands.registry.register: cmd.action must be a function')
  assert(_valid_category[cmd.category],
    string.format('commands.registry.register: invalid category %q for id %q (must be one of: %s)',
      tostring(cmd.category), cmd.id, table.concat(M.CATEGORIES, ', ')))
  if M._commands[cmd.id] then
    error(string.format('commands.registry.register: id %q already registered', cmd.id), 2)
  end
  M._seq = M._seq + 1
  cmd._seq = M._seq
  M._commands[cmd.id] = cmd
end

--- Remove a command by id. No-op if it isn't registered.
function M.unregister(id)
  M._commands[id] = nil
end

--- Look up a single command by id.
-- @return command table, or nil
function M.get(id)
  return M._commands[id]
end

--- List commands, optionally filtered to one category.
-- Order is registration order (stable), not alphabetical — built-in
-- commands are registered in their curated menu order (see
-- modules/commands/bootstrap.lua), which reads better in a palette than
-- an alphabetical shuffle, and keeps id "position 1" predictable.
-- @param category (optional) one of M.CATEGORIES
-- @return array of command tables, in registration order
function M.list(category)
  local out = {}
  for _, cmd in pairs(M._commands) do
    if not category or cmd.category == category then out[#out + 1] = cmd end
  end
  table.sort(out, function(a, b) return a._seq < b._seq end)
  return out
end

--- Return the fixed category list (in display order).
function M.categories()
  return M.CATEGORIES
end

return M
