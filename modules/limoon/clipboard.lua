-- Copyright 2025-2026 Mitchell. See LICENSE.

if not CURSES then return nil end

--- Allows the terminal version's buffer clipboard functions to operate on the system clipboard.
-- This module is only enabled in the terminal version.
-- @module limoon.clipboard
local M = {}

-- Helper function to check if a command exists
local function command_exists(cmd)
	local f = io.popen(cmd .. ' 2>/dev/null', 'r')
	if not f then return false end
	f:close()
	return true
end

-- Check if we have a display/clipboard environment available
local function has_clipboard_env()
	if WIN32 or OSX then return true end
	local display = os.getenv('DISPLAY')
	local wayland = os.getenv('WAYLAND_DISPLAY')
	return display ~= nil or wayland ~= nil
end

--- The command to retrieve the system clipboard's contents.
-- The default values are:
-- - Windows: `powershell get-clipboard`
-- - macOS: `pbpaste`
-- - Linux/BSD: `xsel -b -o` or `wl-paste -n` if available.
M.paste_command = nil

--- The command to modify the system clipboard's contents.
-- The default values are:
-- - Windows: `clip`
-- - macOS: `pbcopy`
-- - Linux/BSD: `xsel -n -b -i` or `wl-copy -f` if available.
M.copy_command = nil

-- Detect available clipboard commands
if has_clipboard_env() then
	if LINUX or BSD then
		if command_exists('xsel -b -o') then
			M.paste_command = 'xsel -b -o'
			M.copy_command = 'xsel -n -b -i'
		elseif command_exists('wl-paste -n') then
			M.paste_command = 'wl-paste -n'
			M.copy_command = 'wl-copy -f'
		end
	elseif OSX then
		M.paste_command = 'pbpaste'
		M.copy_command = 'pbcopy'
	elseif WIN32 then
		M.paste_command = 'powershell get-clipboard'
		M.copy_command = 'clip'
	end
end

-- Internal clipboard storage for cross-buffer copy/paste
local internal_clipboard = nil

-- Get text from system clipboard
local function get_system_clipboard()
	if not M.paste_command then return nil end
	
	local proc = os.spawn(M.paste_command)
	if not proc then return nil end
	
	local text = proc:read('a')
	if not text or text == '' then return nil end
	
	-- Remove trailing newline from powershell
	if WIN32 then text = text:gsub('\r?\n$', '') end
	
	return text
end

-- Copy text to system clipboard
local function set_system_clipboard(text)
	if not M.copy_command or not text then return end
	
	local proc = os.spawn(M.copy_command)
	if not proc then return end
	
	proc:write(text)
	proc:close()
	
	-- Wait for process on Windows/Mac, timeout on Linux
	if WIN32 or OSX then
		local start = os.time()
		while proc:status() == 'running' do
			if os.difftime(os.time(), start) > 2 then
				proc:kill(9)
				break
			end
		end
	else
		timeout(1, function() proc:kill(9) end)
	end
end

local get_scintilla_clipboard = ui.get_clipboard_text

-- Documentation is in core/ui.lua.
function ui.get_clipboard_text(internal)
	if internal then return get_scintilla_clipboard() end
	
	-- Try system clipboard first
	if M.paste_command then
		local text = get_system_clipboard()
		if text then return text end
	end
	
	-- Fall back to internal clipboard
	return get_scintilla_clipboard()
end

-- Replaces buffer clipboard functions with functions that use the system clipboard.
local function enable_system_clipboard()
	-- Capture original functions
	local orig_paste = buffer.paste
	local orig_copy = buffer.copy
	local orig_cut = buffer.cut
	local orig_copy_text = buffer.copy_text
	
	-- Copy: save to both internal and system clipboard
	buffer.copy = function(self)
		orig_copy(self)
		internal_clipboard = ui.get_clipboard_text(true)
		set_system_clipboard(internal_clipboard)
	end
	
	-- Cut: same as copy
	buffer.cut = function(self)
		orig_cut(self)
		internal_clipboard = ui.get_clipboard_text(true)
		set_system_clipboard(internal_clipboard)
	end
	
	-- Copy text directly
	buffer.copy_text = function(self, text)
		orig_copy_text(self, text)
		internal_clipboard = text
		set_system_clipboard(text)
	end
	
	-- Paste: try system clipboard first, fall back to internal
	buffer.paste = function(self)
		local text = get_system_clipboard()
		
		if not text or text == '' then
			text = internal_clipboard
		else
			-- Update internal for cross-buffer paste
			internal_clipboard = text
		end
		
		if text and text ~= '' then
			-- Use add_text which is safer than native paste
			local ok = pcall(function() self:add_text(text) end)
			if not ok then orig_paste(self) end
		else
			orig_paste(self)
		end
	end
end

enable_system_clipboard()
events.connect(events.BUFFER_NEW, enable_system_clipboard)

return M
