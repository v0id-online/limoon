-- Copyright 2020-2025 Mitchell. See LICENSE.

--- Load localizations from a locale file and return them in a table.
-- @param locale_conf String path to a local file to load.
local function load_locale(locale_conf)
	local L = {}
	for line in io.lines(locale_conf) do
		if line:find('^%s*[^%w_%[]') then goto continue end -- comment
		local id, str = line:match('^(.-)%s*=%s*(.+)$')
		if id and str and test.assert(not L[id], 'duplicate locale id "%s"', id) then L[id] = str end
		::continue::
	end
	return L
end

--- Loads additional localizations read from assignments in a Lua file.
-- @param filename String path to a Lua file to read localizations from.
-- @param L Table of localizations to add to.
local function load_extra_localizations(L, filename)
	local count, i = 0, 1
	for line in io.lines(filename) do
		if not line:find('_L%b[]%s*=') then goto continue end
		for id in line:gmatch([=[_L%[['"]([^'"]+)['"]%]%s*=]=]) do
			test.assert(not L[id], 'duplicate locale id "%s" on line %d (first seen in %s)', id, i, L[id])
			L[id], count = string.format('%s:%d', filename:sub(#_HOME + 2), i), count + 1
		end
		::continue::
		i = i + 1
	end
	if count > 0 then test.log(string.format('Added %d localizations.', count)) end
end

--- Looks for use of localization in the given Lua file and returns a list of missing IDs.
-- @param filename String filename of the Lua file to check.
-- @param L Table of localizations to read from.
-- @return list of missing locale IDs
local function check_missing_localizations(filename, L)
	local missing = {}
	local count = 0
	for line in io.lines(filename) do
		for id in line:gmatch([=[_L%[['"]([^'"]+)['"]%]]=]) do
			if not L[id] then missing[#missing + 1] = id end
			count = count + 1
		end
	end
	if count > 0 then test.log(string.format('Checked %d localizations.', count)) end
	return missing
end

local L = load_locale(_HOME .. '/core/locale.conf')

-- Test each of the locale.conf files.
for locale_conf in lfs.walk(_HOME .. '/core/locales') do
	test(locale_conf:match('[^/\\]+$') .. ' should have the same locale IDs as locale.conf',
		function()
			local missing = {}
			local extra = {}

			local l = load_locale(locale_conf)
			for id in pairs(L) do if not l[id] then missing[#missing + 1] = id end end
			for id in pairs(l) do if not L[id] then extra[#extra + 1] = id end end

			test.assert_equal(missing, {})
			test.assert_equal(extra, {})
		end)
end

local stock_files = {}
local filter = {'*.lua', 'core/*.lua', 'modules/textadept/*.lua', '!**/*_test.lua'}
for filename in lfs.walk(_HOME, filter) do stock_files[#stock_files + 1] = filename:gsub('\\', '/') end
table.sort(stock_files)

-- Test each stock file.
for _, stock_file in ipairs(stock_files) do
	test(stock_file:sub(#_HOME + 2) .. ' should be using known locale IDs', function()
		local missing = check_missing_localizations(stock_file, L)

		test.assert_equal(missing, {})
	end)
end

local extra_files = {}
filter = {
	'**/*.lua', '!textadept', '!**/build', '!**/*_test.lua',
	-- Extra modules.
	'!**/{dkjson,mobdebug,socket}.lua', '!lsp/{ldoc,logging,pl}'
}
for filename in lfs.walk(_HOME .. '/modules', filter) do extra_files[#extra_files + 1] = filename end
table.sort(extra_files)

-- Test each extra file.
for _, extra_file in ipairs(extra_files) do
	test(extra_file:sub(#_HOME + 2) .. ' should be using known locale IDs', function()
		load_extra_localizations(L, extra_file)

		local missing = check_missing_localizations(extra_file, L)

		test.assert_equal(missing, {})
	end)
	retry(0) -- do not retry or else there will be erroneous 'duplicate locale id' errors
end
