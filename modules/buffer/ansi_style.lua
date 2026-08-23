-- ansi_style.lua — Parse ANSI SGR escape sequences into Scintilla styles.
--
-- Converts a string containing ANSI "Select Graphic Rendition" escapes
-- (the color/bold/italic/underline codes terminal programs like `glow`
-- emit — ESC '[' params 'm') into plain text plus a list of style spans
-- describing which byte ranges get which attributes. Does not attempt
-- cursor movement, screen-clearing, or any other non-SGR escape — glow's
-- non-interactive (`-s <style>`, not `-t`) output only ever emits SGR.
--
-- Usage:
--   local ansi_style = require('buffer.ansi_style')
--   local plain, spans = ansi_style.parse(rendered_with_ansi)
--   ansi_style.apply(buffer, view, spans, style_base)

local M = {}

--- First style number this module allocates from, and how many it will
-- use at most (200..239). Scintilla's fixed styles occupy 32..39
-- (STYLE_DEFAULT..STYLE_FOLDDISPLAYTEXT — see Scintilla.h's STYLE_MAX of
-- 255); a real lexer's own tag-based styles are numbered from low values
-- upward (core/lexer.lua's buffer.lexer._TAGS), so starting near the top
-- of the 0..255 range keeps collisions unlikely for any lexer with a
-- realistic tag count. This is a heuristic, not a guarantee: a view that
-- later shows a lexer using 160+ distinct tags could still collide with
-- these slots. Accepted as a known limitation — full collision-proofing
-- would need runtime introspection Scintilla doesn't expose.
M.style_base = 200
M.max_slots = 40

--- xterm's standard 16-color palette (0-7 normal, 8-15 bright), as
-- packed 0xBBGGRR values (Scintilla's native color format — blue is the
-- high byte, red is the low byte).
local function pack(r, g, b) return r + g * 256 + b * 65536 end

local PALETTE_16 = {
  pack(0, 0, 0), pack(205, 0, 0), pack(0, 205, 0), pack(205, 205, 0),
  pack(0, 0, 238), pack(205, 0, 205), pack(0, 205, 205), pack(229, 229, 229),
  pack(127, 127, 127), pack(255, 0, 0), pack(0, 255, 0), pack(255, 255, 0),
  pack(92, 92, 255), pack(255, 0, 255), pack(0, 255, 255), pack(255, 255, 255),
}

--- xterm's 256-color palette: 0-15 basic/bright, 16-231 a 6x6x6 color
-- cube, 232-255 a grayscale ramp. Standard xterm level formula.
local function palette_256(n)
  if n < 16 then return PALETTE_16[n + 1] end
  if n < 232 then
    local i = n - 16
    local function level(x) return x == 0 and 0 or 55 + x * 40 end
    local r, g, b = math.floor(i / 36), math.floor((i % 36) / 6), i % 6
    return pack(level(r), level(g), level(b))
  end
  local v = 8 + (n - 232) * 10
  return pack(v, v, v)
end

--- Apply one SGR sequence's semicolon-separated params to style state
-- *cur* (in place). Unrecognized codes are ignored.
-- @param cur Table with fg, bg, bold, italic, underline fields.
-- @param params The raw digits-and-semicolons string between '[' and 'm'.
local function apply_sgr(cur, params)
  local toks = {}
  for t in (params .. ';'):gmatch('([^;]*);') do toks[#toks + 1] = tonumber(t) or 0 end
  if #toks == 0 then toks = {0} end -- bare "\27[m" means reset, same as "\27[0m"

  local i = 1
  while i <= #toks do
    local code = toks[i]
    if code == 0 then
      cur.fg, cur.bg, cur.bold, cur.italic, cur.underline = nil, nil, false, false, false
    elseif code == 1 then
      cur.bold = true
    elseif code == 22 then
      cur.bold = false
    elseif code == 3 then
      cur.italic = true
    elseif code == 23 then
      cur.italic = false
    elseif code == 4 then
      cur.underline = true
    elseif code == 24 then
      cur.underline = false
    elseif code == 39 then
      cur.fg = nil
    elseif code == 49 then
      cur.bg = nil
    elseif code >= 30 and code <= 37 then
      cur.fg = PALETTE_16[code - 30 + 1]
    elseif code >= 90 and code <= 97 then
      cur.fg = PALETTE_16[code - 90 + 8 + 1]
    elseif code >= 40 and code <= 47 then
      cur.bg = PALETTE_16[code - 40 + 1]
    elseif code >= 100 and code <= 107 then
      cur.bg = PALETTE_16[code - 100 + 8 + 1]
    elseif code == 38 or code == 48 then
      local mode = toks[i + 1]
      local color
      if mode == 5 then
        color = palette_256(toks[i + 2] or 0)
        i = i + 2
      elseif mode == 2 then
        color = pack(toks[i + 2] or 0, toks[i + 3] or 0, toks[i + 4] or 0)
        i = i + 4
      end
      if color then
        if code == 38 then cur.fg = color else cur.bg = color end
      end
    end
    i = i + 1
  end
end

--- Parse ANSI SGR escapes out of *text*.
-- @param text String possibly containing "\27[...m" sequences.
-- @return plain (text with escapes removed), spans (array of
--   {len = N, style = styleTableOrNil} covering all of plain,
--   in order; style is nil for plain/unstyled runs, otherwise a table
--   with fg/bg/bold/italic/underline fields — identical attribute
--   combinations share the same style table by reference, so callers
--   can key a style-number cache off that identity).
function M.parse(text)
  local plain_parts = {}
  local spans = {}
  local style_cache = {}

  local cur = {fg = nil, bg = nil, bold = false, italic = false, underline = false}
  local cur_len = 0

  local function flush_span()
    if cur_len == 0 then return end
    local has_attrs = cur.fg or cur.bg or cur.bold or cur.italic or cur.underline
    local style = nil
    if has_attrs then
      local key = table.concat(
        {tostring(cur.fg), tostring(cur.bg), tostring(cur.bold), tostring(cur.italic), tostring(cur.underline)}, '|')
      style = style_cache[key]
      if not style then
        style = {fore = cur.fg, back = cur.bg, bold = cur.bold, italic = cur.italic, underline = cur.underline}
        style_cache[key] = style
      end
    end
    spans[#spans + 1] = {len = cur_len, style = style}
    cur_len = 0
  end

  local pos, n = 1, #text
  while pos <= n do
    local s, e, params = text:find('\27%[([0-9;]*)m', pos)
    if not s then
      local literal = text:sub(pos)
      plain_parts[#plain_parts + 1] = literal
      cur_len = cur_len + #literal
      break
    end
    if s > pos then
      local literal = text:sub(pos, s - 1)
      plain_parts[#plain_parts + 1] = literal
      cur_len = cur_len + #literal
    end
    flush_span()
    apply_sgr(cur, params)
    pos = e + 1
  end
  flush_span()

  return table.concat(plain_parts), spans
end

--- Apply parsed *spans* to *buf*'s content in *v* (a view), styling from
-- position 0. Allocates style numbers from M.style_base up to
-- M.max_slots, reusing one number per distinct style table (by
-- identity, as produced by M.parse). Spans with no style (plain runs)
-- use view.STYLE_DEFAULT directly. Unset fore/back on a styled span
-- inherit STYLE_DEFAULT's colors, so bold-only/italic-only spans don't
-- fall back to Scintilla's raw black-on-white default.
-- @param buf Scintilla buffer object (must already contain the matching
--   plain text — see M.parse).
-- @param v View currently showing *buf*.
-- @param spans As returned by M.parse.
function M.apply(buf, v, spans)
  local default_fore = v.style_fore[v.STYLE_DEFAULT]
  local default_back = v.style_back[v.STYLE_DEFAULT]

  local assigned = {} -- style table (by identity) -> style number
  local next_slot = M.style_base

  local function style_num_for(style)
    if not style then return v.STYLE_DEFAULT end
    local num = assigned[style]
    if num then return num end
    if next_slot >= M.style_base + M.max_slots then return v.STYLE_DEFAULT end -- pool exhausted
    num = next_slot
    next_slot = next_slot + 1
    assigned[style] = num
    v.style_fore[num] = style.fore or default_fore
    v.style_back[num] = style.back or default_back
    v.style_bold[num] = style.bold or false
    v.style_italic[num] = style.italic or false
    v.style_underline[num] = style.underline or false
    return num
  end

  buf:start_styling(0, 0)
  for _, span in ipairs(spans) do buf:set_styling(span.len, style_num_for(span.style)) end
end

return M
