-- cmdentry_ext.lua — CURSES-friendly Lua command-entry Tab completion.
-- Intercepts Tab via a high-priority KEYPRESS handler so complete_lua
-- (which calls auto_c_show → crash in notcurses) is never reached.

-- ── Completion state ────────────────────────────────────────────────────────

local _c = { list = {}, idx = 0, trigger = '', symbol = '', op = '' }

-- ── Abbreviated env (mirrors command_entry.lua's env) ───────────────────────

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
    return _G[k] or (ok and v) or ui[k] or
           (textadept and textadept[k])
  end
})

-- ── Completion builder ───────────────────────────────────────────────────────

local function safe_iter(t)
  if type(t) ~= 'table' then return function() end end
  return pairs(t)
end

local function build_completions(symbol, op, part)
  local seen, list = {}, {}
  local patt = '^' .. (part:gsub('[%^%$%(%)%%%.%[%]%*%+%-%?]', '%%%1'))

  local function add(k)
    if type(k) == 'string' and not seen[k] and k:find(patt) then
      seen[k] = true
      list[#list + 1] = k
    end
  end

  if symbol == '' or symbol == 'buffer' or symbol == 'view' then
    for _, t in ipairs { buffer, view, ui, textadept } do
      for k in safe_iter(t) do add(k) end
    end
    -- add matching globals (avoid iterating _G which is huge)
    for k in pairs(_G) do add(k) end
  else
    local f = load('return (' .. symbol .. ')', nil, 't', _env)
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

local function parse_context()
  local line = ui.command_entry:get_text()
  local pos   = ui.command_entry.current_pos  -- byte offset from start of doc
  local before = line:sub(1, pos)
  local symbol, op, part = before:match('([%w_.]-)([%.:]?)([%w_]*)$')
  symbol, op, part = symbol or '', op or '', part or ''
  local prefix = before:sub(1, -(#part + 1))
  return symbol, op, part, prefix
end

-- ── Tab completion handler ───────────────────────────────────────────────────

local function complete()
  local ok, err = pcall(function()
    local symbol, op, part, prefix = parse_context()
    local trigger = prefix   -- prefix already contains symbol+op

    if trigger ~= _c.trigger or symbol ~= _c.symbol or op ~= _c.op then
      _c.list    = build_completions(symbol, op, part)
      _c.idx     = 0
      _c.trigger = trigger
      _c.symbol  = symbol
      _c.op      = op
    end

    if #_c.list == 0 then
      ui.statusbar_text = string.format('No completions for "%s"', part)
      return
    end

    _c.idx = _c.idx % #_c.list + 1

    ui.command_entry:set_text(trigger .. _c.list[_c.idx])
    ui.command_entry:line_end()

    local parts = {}
    for i, s in ipairs(_c.list) do
      parts[#parts + 1] = i == _c.idx and ('[' .. s .. ']') or s
    end
    ui.statusbar_text = string.format('[%d/%d] %s',
      _c.idx, #_c.list, table.concat(parts, '  '))
  end)
  if not ok then
    ui.statusbar_text = 'Completion error: ' .. tostring(err)
  end
end

-- ── Reset state when command entry opens ────────────────────────────────────

local _orig_run = ui.command_entry.run
ui.command_entry.run = function(...)
  _c = { list = {}, idx = 0, trigger = '', symbol = '', op = '' }
  _orig_run(...)
end

-- ── Intercept Tab before key module can call complete_lua ───────────────────
-- Priority 1 fires before the default KEYPRESS handler (which consults the
-- key table and would call complete_lua → auto_c_show → crash in notcurses).

events.connect(events.KEYPRESS, function(key)
  if keys.mode ~= '_command_entry' then return end
  if key ~= '\t' then return end
  complete()
  return true  -- halt: Tab never reaches Scintilla or complete_lua
end, 1)
