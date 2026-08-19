-- Copyright 2025-2026 Mitchell. See LICENSE.

if not CURSES then return nil end

--- Allows the terminal version's buffer clipboard functions to operate on the system clipboard.
-- This module is only enabled in the terminal version.
-- @module limoon.clipboard
local M = {}

-- Helper function to check if a command exists.
-- io.popen always returns a handle for the SHELL, even when `cmd` itself
-- doesn't exist — it can only fail to launch a shell at all, which is rare.
-- So the previous implementation reported every command as available,
-- silently breaking system clipboard detection whenever the underlying
-- tool (xsel, wl-copy, ...) was actually missing.
local function command_exists(cmd)
	-- `command -v` is a POSIX shell builtin that succeeds only if the
	-- command exists, and only test the first token (the binary itself):
	-- `command -v 'xsel -b -o'` would fail since that's not a binary name.
	local first_token = cmd:match('^%S+')
	if not first_token then return false end
	local ok = os.execute('command -v ' .. first_token .. ' >/dev/null 2>&1')
	return ok == true
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
			-- No -n/--nodetach: xsel needs to fork into the background and
			-- keep running to actually OWN and serve the X CLIPBOARD
			-- selection. With -n it stays attached to our process and the
			-- selection is abandoned the instant we close/kill it below —
			-- copy silently does nothing for any other app, and even our
			-- own next paste finds no selection owner.
			M.copy_command = 'xsel -b -i'
		elseif command_exists('wl-paste -n') then
			M.paste_command = 'wl-paste -n'
			-- Same reasoning as xsel above: -f/--foreground stops wl-copy
			-- from forking to the background, so it can't persist the
			-- Wayland clipboard past our own process closing/killing it.
			M.copy_command = 'wl-copy'
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

-- Capture originals once, at module load. Never re-wrap: enable_system_clipboard
-- previously re-read buffer.paste/copy/cut/copy_text on every call, but since it
-- runs on every events.BUFFER_NEW, and it had already replaced those fields on
-- the first call, each new buffer wrapped the PREVIOUS wrapper instead of the
-- true original — stacking N nested calls after N buffers were opened.
--
-- keys.lua and menu.lua both bind Ctrl+C/Ctrl+X (and the Edit menu's Cut/Copy)
-- to buffer.copy_allow_line/cut_allow_line, NOT plain buffer.copy/cut — so
-- those (not copy/cut) are what actually needs wrapping for the normal
-- keyboard/menu path to reach the system clipboard at all. copy/cut are kept
-- wrapped too, for any other code that calls them directly by name.
local orig_paste           = buffer.paste
local orig_copy            = buffer.copy
local orig_cut             = buffer.cut
local orig_copy_text       = buffer.copy_text
local orig_copy_allow_line = buffer.copy_allow_line
local orig_cut_allow_line  = buffer.cut_allow_line

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
-- Installs onto the shared `buffer` table, so it applies to all buffers without
-- needing to run again per-buffer (see orig_* capture above).
--
-- keys.lua binds these bare (e.g. `keys['ctrl+v'] = buffer.paste`, called as
-- `key()` with NO arguments — see core/keys.lua's key_command()), so `self`
-- is nil whenever invoked that way. The native SCI-backed methods tolerate a
-- nil self just fine (call_scintilla_lua in src/limoon.c falls back to the
-- global `focused_view` when no buffer/view argument is given) — but plain
-- Lua indexing like `self.selection_empty` on a nil self throws immediately.
-- Every function below uses `self or buffer` (the global buffer proxy, which
-- itself resolves to focused_view) so they work both bare and when called
-- with an explicit target via colon syntax (e.g. command_entry.lua's
-- `M:copy_allow_line()`).
local function enable_system_clipboard()
	-- Copy: save to both internal and system clipboard
	buffer.copy = function(self)
		local target = self or buffer
		orig_copy(target)
		internal_clipboard = ui.get_clipboard_text(true)
		set_system_clipboard(internal_clipboard)
	end

	-- Cut: same as copy
	buffer.cut = function(self)
		local target = self or buffer
		orig_cut(target)
		internal_clipboard = ui.get_clipboard_text(true)
		set_system_clipboard(internal_clipboard)
	end

	-- Copy-allow-line: same as copy, but this is what Ctrl+C and the Edit
	-- menu's "Copy" actually invoke (see keys.lua/menu.lua) — copy/cut above
	-- are never reached through the normal keyboard/menu path at all.
	buffer.copy_allow_line = function(self)
		local target = self or buffer
		orig_copy_allow_line(target)
		internal_clipboard = ui.get_clipboard_text(true)
		set_system_clipboard(internal_clipboard)
	end

	-- Cut-allow-line: same as cut, but this is what Ctrl+X and the Edit
	-- menu's "Cut" actually invoke.
	buffer.cut_allow_line = function(self)
		local target = self or buffer
		orig_cut_allow_line(target)
		internal_clipboard = ui.get_clipboard_text(true)
		set_system_clipboard(internal_clipboard)
	end

	-- Copy text directly
	buffer.copy_text = function(self, text)
		local target = self or buffer
		orig_copy_text(target, text)
		internal_clipboard = text
		set_system_clipboard(text)
	end

	-- Paste: try system clipboard first, fall back to internal
	buffer.paste = function(self)
		local target = self or buffer
		local text = get_system_clipboard()

		if not text or text == '' then
			text = internal_clipboard
		else
			-- Update internal for cross-buffer paste
			internal_clipboard = text
		end

		if text and text ~= '' then
			-- Insert if there's no selection; otherwise replace it — add_text
			-- alone only inserts, so pasting over a selection used to leave
			-- the old selected text in place with the paste appended after it.
			local ok
			if target.selection_empty then
				ok = pcall(function() target:add_text(text) end)
			else
				ok = pcall(function() target:replace_sel(text) end)
			end
			if not ok then orig_paste(target) end
		else
			orig_paste(target)
		end
	end
end

enable_system_clipboard()

return M
