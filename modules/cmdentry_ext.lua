-- cmdentry_ext.lua — CURSES-friendly Lua command-entry enhancements.
-- Replaces Scintilla's popup autocomplete (which crashes in the terminal)
-- with a statusbar-based cycling completion.
--
-- Tab     → cycle forward through completions (statusbar shows the list)
-- Shift+Tab → NOT bound here (Shift+Tab inserts literal tab in CE), but
--             you can add keys._command_entry['shift+\t'] = complete_prev
-- After CE closes, the completion state is reset automatically.

local M = ui.command_entry

-- ── Completion state ────────────────────────────────────────────────────────

local _c = { list = {}, idx = 0, trigger = '' }

-- ── Abbreviated env (mirrors command_entry.lua's env) ───────────────────────
-- Used to evaluate the symbol prefix (e.g. "textadept.themes") safely.

local _env = setmetatable({}, {
  __index = function(_, k)
    local ok, v = pcall(function() return buffer[k] end)
    if ok and type(v) == 'function' then
      return function(...) return buffer[k](buffer, ...) end
    end
    ok, v = pcall(function() return view[k] end)
    if ok and type(v) == 'function' then
      return function(...) return view[k](view, ...) end
    end
    return (ok and v) or view[k] or ui[k] or _G[k] or
           (textadept and textadept[k])
  end
})

-- ── Completion builder ───────────────────────────────────────────────────────

-- Safely iterate a value (skips non-tables gracefully).
local function safe_iter(t)
  if type(t) ~= 'table' then return function() end end
  return pairs(t)
end

-- Build a sorted list of completion strings for the given (symbol, op, part).
local function build_completions(symbol, op, part)
  local seen, list = {}, {}
  local patt = '^' .. part

  local function add(k)
    if type(k) == 'string' and not seen[k] and k:find(patt) then
      seen[k] = true
      list[#list + 1] = k
    end
  end

  if symbol == '' or symbol == 'buffer' or symbol == 'view' then
    -- Complete from the abbrevieted global env
    for _, t in ipairs { buffer, view, ui, _G, textadept } do
      for k in safe_iter(t) do add(k) end
    end
  else
    -- Complete from the symbol's members
    local f = load('return ' .. symbol, nil, 't', _env)
    if f then
      local ok, result = pcall(f)
      if ok and type(result) == 'table' then
        for k, v in pairs(result) do
          if op == '.' or type(v) == 'function' then add(k) end
        end
      end
    end
  end

  table.sort(list)
  return list
end

-- ── Parse cursor context ─────────────────────────────────────────────────────

-- Returns symbol, op, part, and the text before the partial word.
local function parse_context()
  local line, pos = M:get_cur_line()
  local before    = line:sub(1, pos - 1)
  local symbol, op, part = before:match('([%w_.]-)([%.:]?)([%w_]*)$')
  symbol, op, part = symbol or '', op or '', part or ''
  -- prefix = everything before the partial word (symbol + op included)
  local prefix = before:sub(1, -(#part + 1))
  return symbol, op, part, prefix
end

-- ── Tab completion handler ───────────────────────────────────────────────────

local function complete()
  local symbol, op, part, prefix = parse_context()
  local trigger = prefix .. symbol .. op

  -- Rebuild list when the context changes
  if trigger ~= _c.trigger then
    _c.list    = build_completions(symbol, op, part)
    _c.idx     = 0
    _c.trigger = trigger
  end

  if #_c.list == 0 then
    ui.statusbar_text = string.format('No completions for "%s"', part)
    return
  end

  -- Cycle forward
  _c.idx = _c.idx % #_c.list + 1

  -- Insert completion into entry
  M:set_text(trigger .. _c.list[_c.idx])
  M:line_end()

  -- Show annotated list in statusbar: [selected] others…
  local parts = {}
  for i, s in ipairs(_c.list) do
    parts[#parts + 1] = i == _c.idx and ('[' .. s .. ']') or s
  end
  ui.statusbar_text = string.format('[%d/%d] %s',
    _c.idx, #_c.list, table.concat(parts, '  '))
end

-- ── Hook into ui.command_entry.run ──────────────────────────────────────────

local _orig_run = ui.command_entry.run
ui.command_entry.run = function(label, ...)
  local was_active = keys.mode == '_command_entry'
  _orig_run(label, ...)
  -- Only override Tab for the default no-label Lua command entry
  if not label and not was_active and keys._command_entry then
    keys._command_entry['\t'] = complete
    -- Reset completion state for fresh session
    _c = { list = {}, idx = 0, trigger = '' }
  end
end
