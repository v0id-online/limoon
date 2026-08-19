-- language.lua — "Select Language" command (Ctrl+P palette).
--
-- Li Moon auto-detects the UI language from $LANG (see core/locale.lua)
-- and loads a matching core/locales/locale.<code>.conf translation file
-- if one exists. This command lets the user force English regardless of
-- $LANG, or restore the automatic detection.
--
-- The override mechanism is core/locale.lua's own: it checks
-- _USERHOME/locale.conf BEFORE the $LANG-detected file, and stops at the
-- first file that exists — even an EMPTY one. So an empty override file
-- forces English (every _L[key] falls through to the raw English key,
-- since _L's __index returns the key unchanged when no translation is
-- loaded); removing the override file restores $LANG auto-detection.
--
-- _L only gets built once, at Lua init, so a language change here needs
-- a restart to take effect — same as the UI color picker.

local commands = require('commands.registry')

local FRIENDLY_NAMES = {
	ar = 'العربية (Arabic)', de = 'Deutsch (German)', es = 'Español (Spanish)',
	fr = 'Français (French)', it = 'Italiano (Italian)', ja = '日本語 (Japanese)',
	pl = 'Polski (Polish)', pt_BR = 'Português (Brazilian Portuguese)',
	pt = 'Português (Portuguese)', ru = 'Русский (Russian)',
	sv = 'Svenska (Swedish)', zh = '中文 (Chinese)',
}

local OVERRIDE_FILE = _USERHOME .. '/locale.conf'

-- Mirrors core/locale.lua's own $LANG parsing, so the two agree on which
-- locale would be auto-selected.
local function detect_locale()
	local locale, lang = (os.getenv('LANG') or ''):match('^(([^_.@]+)_?[^.@]*)')
	if not locale then return nil end
	for _, code in ipairs{locale, lang} do
		if code and lfs.attributes(string.format('%s/core/locales/locale.%s.conf', _HOME, code)) then
			return code, FRIENDLY_NAMES[code] or code
		end
	end
	return nil
end

local function select_language()
	local code, name = detect_locale()
	local override_exists = lfs.attributes(OVERRIDE_FILE) ~= nil
	local using_english = override_exists or not code

	local items = {'English' .. (using_english and '  [current]' or '')}
	local choices = {'en'}
	if code then
		items[#items + 1] = name .. (not using_english and '  [current, from $LANG]' or '')
		choices[#choices + 1] = code
	end

	local i = ui.dialogs.list{title = 'Select Language', columns = {'Language'}, items = items}
	if not i or not choices[i] then return end

	if choices[i] == 'en' then
		local f = io.open(OVERRIDE_FILE, 'w')
		if f then f:close() end
	else
		os.remove(OVERRIDE_FILE)
	end
	ui.statusbar_text = 'Language set to ' .. items[i]:gsub('%s*%[.-%]$', '') ..
		'. Restart Li Moon to apply.'
end

commands.register{
	id = 'view.select_language',
	name = 'Select Language',
	description = 'Force English, or restore automatic $LANG-based language detection',
	category = 'View',
	action = select_language,
}

return {}
