-- ui_widgets.lua — Reusable UI components for Li Moon plugins.
--
-- Usage in a plugin:
--   local W = require('ui_widgets')
--   W.status_add('my_plugin', function() return 'Hello' end, 20)
--   W.notify('Done!', 3)

local M = {}

-- ── Statusbar segments ──────────────────────────────────────────────
-- Plugins register functions that return short strings; the widget
-- library concatenates them into ui.statusbar_text (bar 0, left side).

M._segments = {}   -- [id] = {fn=function, priority=number}
local _pending_restore = nil   -- text to restore after a notification

--- Register a status segment.
-- fn() should return a string (or '' / nil to hide).
-- Lower priority values appear first (leftmost).
function M.status_add(id, fn, priority)
  M._segments[id] = {fn = fn, priority = priority or 50}
end

--- Unregister a status segment.
function M.status_remove(id)
  M._segments[id] = nil
end

--- Build and apply the combined statusbar text.
function M.status_refresh()
  if _pending_restore then return end  -- a notification is active, don't clobber it
  local ids = {}
  for id in pairs(M._segments) do ids[#ids + 1] = id end
  table.sort(ids, function(a, b)
    return (M._segments[a].priority or 50) < (M._segments[b].priority or 50)
  end)
  local parts = {}
  for _, id in ipairs(ids) do
    local ok, text = pcall(M._segments[id].fn)
    if ok and text and text ~= '' then parts[#parts + 1] = text end
  end
  ui.statusbar_text = table.concat(parts, '  │  ')
end

-- Refresh on common state-change events.
events.connect(events.BUFFER_AFTER_SWITCH, M.status_refresh)
events.connect(events.FILE_OPENED,         M.status_refresh)
events.connect(events.BUFFER_NEW,          M.status_refresh)

-- ── Notifications ───────────────────────────────────────────────────

--- Show a temporary message in the statusbar.
-- @param text  Message to display.
-- @param secs  Seconds to show it (default: permanent until next event).
function M.notify(text, secs)
  _pending_restore = ui.statusbar_text
  ui.statusbar_text = '  ● ' .. text
  if secs then
    local start = os.time()
    local function check()
      if os.time() - start >= secs then
        ui.statusbar_text = _pending_restore or ''
        _pending_restore = nil
        return false  -- disconnect
      end
    end
    events.connect(events.UPDATE_UI, check)
  end
end

-- ── Modal dialogs ───────────────────────────────────────────────────

--- Single-line text input dialog.
-- @param title   Dialog title.
-- @param prompt  Label shown above the field (optional).
-- @param default Initial text in the field (optional).
-- @return Entered string, or nil if cancelled.
function M.input(title, prompt, default)
  return ui.dialogs.input{title = title or 'Input', text = prompt or ''}
end

--- Yes / No confirmation dialog.
-- @param title  Dialog title.
-- @param msg    Message text.
-- @return true if the user clicked Yes, false otherwise.
function M.confirm(title, msg)
  local btn = ui.dialogs.message{title = title, text = msg,
                                  button1 = 'Yes', button2 = 'No'}
  return btn == 1
end

--- List-picker dialog.
-- @param title    Dialog title.
-- @param items    Flat list of strings.
-- @param columns  Column header(s) table (optional, default {'Item'}).
-- @return Index into items (1-based), or nil if cancelled.
function M.pick(title, items, columns)
  return ui.dialogs.list{title = title, columns = columns or {'Item'},
                          items = items}
end

-- ── Output buffers ──────────────────────────────────────────────────
-- Plugins can render structured text into read-only named buffers.

--- Return (creating if needed) a named output buffer and switch to it.
-- The buffer is identified by its _type field so it survives file switches.
-- @param name  Unique buffer type / display name.
-- @return Scintilla buffer object.
function M.output_buffer(name)
  for _, buf in ipairs(_BUFFERS) do
    if buf._type == name then
      view:goto_buffer(buf)
      return buf
    end
  end
  local buf = buffer.new()
  buf._type = name
  buf.read_only = false
  buf:append_text('')
  buf.read_only = true
  buf:set_save_point()
  return buf
end

--- Write text into an output buffer.
-- @param buf    Buffer object (from output_buffer) or name string.
-- @param text   Text to append (or replace if clear=true).
-- @param clear  If true, erase existing content first.
function M.write(buf, text, clear)
  if type(buf) == 'string' then buf = M.output_buffer(buf) end
  local was_ro = buf.read_only
  buf.read_only = false
  if clear then buf:clear_all() end
  buf:document_end()
  buf:append_text(text)
  buf:set_save_point()
  if was_ro then buf.read_only = true end
end

return M
