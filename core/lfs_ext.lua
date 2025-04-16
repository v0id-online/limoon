-- Copyright 2007-2025 Mitchell. See LICENSE.

--- Extends the `lfs` library to find files in directories and determine absolute file paths.
-- @module lfs

--- The default filter table used when iterating over files and directories using `lfs.walk()`.
-- - File extensions excluded: a, bmp, bz2, class, dll, exe, gif, gz, jar, jpeg, jpg, o, pdf,
--	png, so, tar, tgz, tif, tiff, xz, and zip.
-- - Directories excluded: .bzr, .git, .hg, .svn, \_FOSSIL\_, and node_modules.
-- @table default_filter

-- LuaFormatter off
lfs.default_filter = {--[[Extensions]]'!.a','!.bmp','!.bz2','!.class','!.dll','!.exe','!.gif','!.gz','!.jar','!.jpeg','!.jpg','!.o','!.pdf','!.png','!.so','!.tar','!.tgz','!.tif','!.tiff','!.xz','!.zip',--[[Directories]]'!/.bzr/','!/.git/','!/.hg/','!/.svn/','!/_FOSSIL_/','!/node_modules/'}
-- LuaFormatter on

--- Documentation is in `lfs.walk()`.
-- @param dir
-- @param filter
-- @param n
-- @param include_dirs
-- @param seen Utility table that holds directories seen. If there is a duplicate, stop walking
--	down that path (it is probably a recursive symlink).
-- @param level Utility value indicating the directory level this function is at.
local function walk(dir, filter, n, include_dirs, seen, level)
	if not seen then seen = {} end
	local sep = not WIN32 and '/' or '\\'
	seen[not WIN32 and dir or dir:gsub('/', sep)] = true
	for basename in lfs.dir(dir) do
		if basename:find('^%.%.?$') then goto continue end -- ignore . and ..
		local filename = dir .. (dir ~= '/' and '/' or '') .. basename
		local mode = lfs.attributes(filename, 'mode')
		if mode ~= 'directory' and mode ~= 'file' then goto continue end
		local include = filter.match_any
		if mode == 'file' then
			local ext = filename:match('[^.]+$')
			if ext and not filter.exts[ext] then goto continue end -- ext is rejected
			if not include then include = ext ~= nil end -- ext is accepted
		end
		for _, patt in ipairs(filter) do
			-- Treat exclusive patterns as logical AND.
			if patt:find('^!') and filename:find(patt:sub(2)) then goto continue end
			-- Treat inclusive patterns as logical OR.
			if not include then include = (not patt:find('^!') and filename:find(patt)) end
		end
		if not include then goto continue end
		local os_filename = not WIN32 and filename or filename:gsub('/', sep)
		if mode == 'file' then
			coroutine.yield(os_filename)
		elseif mode == 'directory' then
			local link = lfs.symlinkattributes(filename, 'target')
			if link and seen[lfs.abspath(link .. sep, dir):gsub('[/\\]+$', '')] then goto continue end
			if include_dirs then coroutine.yield(os_filename .. sep) end
			if n and (level or 0) >= n then goto continue end
			walk(filename, filter, n, include_dirs, seen, (level or 0) + 1)
		end
		::continue::
	end
end

--- Returns an iterator that iterates over all files in a directory and its sub-directories.
-- @param dir String directory path to iterate over.
-- @param[opt=lfs.default_filter] filter Filter table or filter string of files to show in the
--	list. A filter consists of glob patterns that match file and directory paths to include
--	or exclude. Patterns are inclusive by default. Exclusive patterns begin with a '!'. If
--	no inclusive patterns are given, any path is initially considered. As a convenience,
--	'/' also matches the Windows directory separator.
-- @param[optchain] n Maximum number of directory levels to descend into. The default
--	is to have no limit.
-- @param[optchain=false] include_dirs Include directory names in iterator results. Directory
--	names will have a trailing '/' or '\\' (depending on the current platform) to distinguish
--	them from regular files.
-- @usage for filename in lfs.walk(buffer.filename:match('^.+[/\\]')) do ... end
function lfs.walk(dir, filter, n, include_dirs)
	dir = assert_type(dir, 'string', 1):match('^..-[/\\]?$')
	assert(lfs.attributes(dir, 'mode') == 'directory', 'directory not found: %s', dir)
	if not assert_type(filter, 'string/table/nil', 2) then filter = lfs.default_filter end
	assert_type(n, 'number/nil', 3)
	-- Process the given filter into something that can match files more easily and/or
	-- quickly. For example, substitute '/' with '[/\\]', and enable hash lookup for file
	-- extensions to include or exclude. Initially allow any extension.
	local filt = {match_any = true, exts = setmetatable({}, {__index = function() return true end})}
	for _, patt in ipairs(type(filter) == 'table' and filter or {filter}) do
		patt = patt:gsub('[.+%()-]', '%%%0'):gsub('%?', '.'):gsub('%*', '.-') -- shell patterns
		patt = patt:gsub('/([^\\])', '[/\\]%1') -- '/' to '[/\\]'
		local include = not patt:find('^!')
		local ext = patt:match('^!?%%.([^.]+)$')
		if ext then
			filt.exts[ext] = include
			if include then setmetatable(filt.exts, nil) end -- disallow any extension
		else
			if include then filt.match_any = false end
			filt[#filt + 1] = patt
		end
	end
	local co = coroutine.create(walk)
	return function() return select(2, coroutine.resume(co, dir, filt, n, include_dirs)) end
end

--- Returns the absolute path to a filename.
-- The returned path is not guaranteed to exist.
-- @param filename String path to a file.
-- @param[opt] prefix String prefix path prepended to a relative filename. The default
--	value is Textadept's current working directory.
function lfs.abspath(filename, prefix)
	assert_type(filename, 'string', 1)
	if WIN32 then filename = filename:gsub('/', '\\'):gsub('^%l:[/\\]', string.upper) end
	if not filename:find(not WIN32 and '^/' or '^%a:[/\\]') and not (WIN32 and filename:find('^\\\\')) then
		if not assert_type(prefix, 'string/nil', 2) then prefix = lfs.currentdir() end
		filename = prefix .. (not WIN32 and '/' or '\\') .. filename
	end
	filename = filename:gsub('%f[^/\\]%.[/\\]', '') -- clean up './'
	local n
	repeat filename, n = filename:gsub('[^/\\]+[/\\]%.%.[/\\]', '', 1) until n == 0 -- clean up '../'
	return filename
end
