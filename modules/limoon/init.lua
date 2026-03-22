-- Copyright 2007-2026 Mitchell. See LICENSE.

--- The limoon module.
-- It provides utilities for editing text in Li Moon.
-- @module limoon
local M = {}
limoon = M -- forward declaration

-- LuaFormatter off
local modules = {'bookmarks','clipboard','command_entry','editing','find','history','macros','run','session','snippets',--[[need to be last]]'menu','keys'}
-- LuaFormatter on
for _, name in ipairs(modules) do M[name] = require('limoon.' .. name) end
M.command_entry, M.find, M.keys = nil, nil, nil -- ui.command_entry, ui.find, unused

return M
