-- Copyright 2007-2025 Mitchell. See LICENSE.

--- Map of all messages used by Textadept to their localized forms.
-- If the localized form of a given message does not exist, the non-localized message is
-- returned. Use Lua's `rawget()` to check if a localization exists.
--
-- Terminal version note: any "_" or "&" mnemonics the GUI version would use are ignored.
-- @module _L
local M = {}

local locale_file = _USERHOME .. '/locale.conf'
if not lfs.attributes(locale_file) then
	local lang = (os.getenv('LANG') or ''):match('^[^_.@]+') -- TODO: support territory (e.g. pt_BR)
	if lang then locale_file = string.format('%s/core/locales/locale.%s.conf', _HOME, lang) end
end
if not lfs.attributes(locale_file) then locale_file = _HOME .. '/core/locale.conf' end
for line in io.lines(locale_file) do
	-- Localization entries must start with a word or '['.
	local id, str = line:match('^([%w_%[].-)%s*=%s*(.-)\r?$')
	if not id then goto continue end
	assert(not M[id], 'duplicate locale key: %s', id)
	M[id] = GTK and str or str:gsub('_', QT and '&' or '')
	::continue::
end

return setmetatable(M, {
	__index = function(_, k) return k end,
	__newindex = QT and function(t, k, v) rawset(t, k, v:gsub('_', '&')) end or nil
})
