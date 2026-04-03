-- Copyright 2007-2026 Mitchell. See LICENSE.

--- Workspace support for Li Moon.
-- A workspace is a named session with a root directory, stored in
-- *_USERHOME/workspaces/&lt;name&gt;/*. Each workspace remembers its project root
-- and the set of open files, views, and cursor positions.
-- @module limoon.workspace
local M = {}

local session = limoon.session
local workspaces_dir = _USERHOME .. '/workspaces'
local session_name = not CURSES and 'session' or 'session_term'

--- The name of the currently active workspace, or `nil` if none.
M.current = nil

--- The root directory of the current workspace, or `nil` if none.
M.dir = nil

-- Returns the session file path for a workspace name.
local function ws_path(name)
	return string.format('%s/%s/%s', workspaces_dir, name, session_name)
end

-- Returns the metadata file path (stores workspace root dir).
local function meta_path(name)
	return workspaces_dir .. '/' .. name .. '/meta.lua'
end

-- Loads workspace metadata from disk.
local function load_meta(name)
	local f = loadfile(meta_path(name), 't', {})
	return f and f() or {}
end

-- Writes workspace metadata to disk.
local function save_meta(name, meta)
	local f = assert(io.open(meta_path(name), 'wb'))
	f:write(string.format('return {dir=%q}', meta.dir or ''))
	f:close()
end

-- Ensures the top-level workspaces directory exists.
local function ensure_workspaces_dir()
	if lfs.attributes(workspaces_dir, 'mode') ~= 'directory' then lfs.mkdir(workspaces_dir) end
end

-- Prompts for a directory. Desktop uses a folder picker; terminal uses text input.
local function ask_directory(title, default)
	if CURSES then
		return ui.dialogs.input{title = title, text = default or lfs.currentdir()}
	end
	return ui.dialogs.open{title = title, dir = default or lfs.currentdir(), only_dirs = true}
end

-- Returns true if filepath is under dir (or if dir is nil/empty).
local function is_under(filepath, dir)
	if not dir or dir == '' then return true end
	local prefix = dir:gsub('[/\\]?$', '/')
	return filepath:sub(1, #prefix) == prefix
end

-- For each open buffer whose file is outside M.dir, ask the user to save it.
local function check_outside_files()
	if not M.dir or M.dir == '' then return end
	for _, buf in ipairs(_BUFFERS) do
		if not buf.filename then goto continue end
		if is_under(buf.filename, M.dir) then goto continue end
		local basename = buf.filename:match('[^/\\]+$') or buf.filename
		local btn = ui.dialogs.message{
			title = 'File Outside Workspace',
			text = string.format("'%s' is outside the workspace directory.\nSave it?", basename),
			icon = 'dialog-question',
			button1 = 'Save As...', button2 = 'Skip'
		}
		if btn == 1 then
			-- Bring this buffer into view, then open save-as dialog.
			for _, v in ipairs(_VIEWS) do
				if v.buffer == buf then ui.goto_view(v); goto switched end
			end
			ui.goto_view(_VIEWS[1])
			view:goto_buffer(buf)
			::switched::
			buffer.save_as()
		end
		::continue::
	end
end

--- Returns a sorted list of existing workspace names.
function M.list()
	local names = {}
	local ok, iter = pcall(lfs.dir, workspaces_dir)
	if not ok then return names end
	for name in iter do
		if name ~= '.' and name ~= '..' then
			if lfs.attributes(workspaces_dir .. '/' .. name, 'mode') == 'directory' then
				names[#names + 1] = name
			end
		end
	end
	table.sort(names)
	return names
end

--- Creates a new empty workspace and makes it the active one.
-- Prompts for a name and a root directory. Saves the current workspace first.
-- @param[opt] name String workspace name. If `nil`, the user is prompted.
function M.new(name)
	if not assert_type(name, 'string/nil', 1) then
		name = ui.dialogs.input{title = 'New Workspace', text = ''}
		if not name or name == '' then return end
	end
	name = name:match('^%s*(.-)%s*$')
	if name == '' then return end
	local ws_dir = workspaces_dir .. '/' .. name
	if lfs.attributes(ws_dir, 'mode') == 'directory' then
		ui.dialogs.message{
			title = 'New Workspace',
			text = string.format("Workspace '%s' already exists.", name),
			icon = 'dialog-warning'
		}
		return
	end
	local dir = ask_directory('Workspace Directory', lfs.currentdir())
	if not dir or dir == '' then return end
	if M.current then session.save(ws_path(M.current)) end
	ensure_workspaces_dir()
	lfs.mkdir(ws_dir)
	save_meta(name, {dir = dir})
	lfs.chdir(dir)
	io.close_all_buffers()
	M.current = name
	M.dir = dir
	session.save(ws_path(name))
	ui.statusbar_text = string.format('Workspace: %s', name)
end

--- Opens a workspace by name, saving the current one first.
-- @param[opt] name String workspace name. If `nil`, the user is prompted with a list.
function M.open(name)
	if not assert_type(name, 'string/nil', 1) then
		local names = M.list()
		if #names == 0 then
			ui.dialogs.message{
				title = 'Open Workspace',
				text = 'No workspaces found. Use "New Workspace" to create one.',
				icon = 'dialog-information'
			}
			return
		end
		name = ui.dialogs.list{title = 'Open Workspace', items = names}
		if not name then return end
	end
	local path = ws_path(name)
	if not lfs.attributes(path) then
		ui.dialogs.message{
			title = 'Open Workspace',
			text = string.format("Workspace '%s' has no saved session.", name),
			icon = 'dialog-warning'
		}
		return
	end
	local meta = load_meta(name)
	-- session.load() auto-saves the previous session_file before switching.
	session.load(path)
	M.current = name
	M.dir = meta.dir or ''
	if M.dir ~= '' and lfs.attributes(M.dir, 'mode') == 'directory' then lfs.chdir(M.dir) end
	ui.statusbar_text = string.format('Workspace: %s', name)
end

--- Saves the current workspace session.
-- Files open outside the workspace directory trigger a save prompt.
-- @param[opt] name String workspace name. Defaults to the current workspace.
function M.save(name)
	name = name or M.current
	if not assert_type(name, 'string/nil', 1) or not name then
		name = ui.dialogs.input{title = 'Save Workspace', text = M.current or ''}
		if not name or name == '' then return end
	end
	check_outside_files()
	ensure_workspaces_dir()
	local ws_dir = workspaces_dir .. '/' .. name
	if lfs.attributes(ws_dir, 'mode') ~= 'directory' then lfs.mkdir(ws_dir) end
	if M.dir then save_meta(name, {dir = M.dir}) end
	session.save(ws_path(name))
	M.current = name
end

--- Deletes a workspace from disk.
-- @param[opt] name String workspace name. If `nil`, the user is prompted with a list.
function M.delete(name)
	if not assert_type(name, 'string/nil', 1) then
		local names = M.list()
		if #names == 0 then return end
		name = ui.dialogs.list{title = 'Delete Workspace', items = names}
		if not name then return end
	end
	local dir = workspaces_dir .. '/' .. name
	if lfs.attributes(dir, 'mode') ~= 'directory' then return end
	local sp, mp = ws_path(name), meta_path(name)
	if lfs.attributes(sp) then os.remove(sp) end
	if lfs.attributes(mp) then os.remove(mp) end
	lfs.rmdir(dir)
	if M.current == name then M.current = nil; M.dir = nil end
end

-- CLI: `li -w name [dir]` opens or creates a workspace on startup.
-- If the workspace does not exist and dir is given, creates it without prompting.
args.register('-w', '--workspace', 2, function(name, dir)
	local path = ws_path(name)
	if lfs.attributes(path) then
		M.open(name)
	elseif dir and dir ~= '' then
		dir = lfs.abspath(dir)
		ensure_workspaces_dir()
		local ws_dir = workspaces_dir .. '/' .. name
		lfs.mkdir(ws_dir)
		save_meta(name, {dir = dir})
		lfs.chdir(dir)
		io.close_all_buffers()
		M.current = name
		M.dir = dir
		session.save(path)
		ui.statusbar_text = string.format('Workspace: %s', name)
	else
		M.open(name) -- will show "no saved session" message
	end
	return true -- prevent events.ARG_NONE from loading the default session
end, 'Load workspace [directory]')

return M
