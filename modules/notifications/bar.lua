-- bar.lua — Notification bar: FIFO queue + level-aware display.
--
-- This module is a foundation piece: it is fully usable on its own (via
-- the stub renderer below) but is NOT wired into any existing Li Moon
-- subsystem yet. Nothing that currently writes to the statusbar (see
-- modules/ui_widgets.lua's M.notify) has been touched or replaced.
--
-- Usage:
--   local notify = require('notifications.bar')
--   notify.success('file_saved', 'init.lua')
--   notify.raw('warning', 'ad-hoc message not in the catalog')

local messages = require('notifications.messages')

local M = {}

--- Max number of queued notifications kept in memory (oldest dropped first).
M.max_queue = 20

--- Seconds each level stays visible before the next queued item shows.
M.durations = {
  info    = 2,
  success = 2,
  warning = 4,
  error   = 6,
}

M._queue = {}  -- FIFO: array of {level, text, time}

--- Render function. Defaults to a stderr stub (dev mode) so the module
-- is testable/usable before the real UI integration exists.
-- TODO(yuri): wire to actual notification plane in src/n_limoon.c once
-- the plane API for overlay/toast rendering is finalized. The renderer
-- signature (level, text) should stay stable across that change.
function M._default_renderer(level, text)
  io.stderr:write(string.format('[notify:%s] %s\n', level, text))
end

M.renderer = M._default_renderer

--- Swap the render function (e.g. to a real notcurses-backed one).
-- @param fn function(level, text)
function M.set_renderer(fn)
  M.renderer = fn or M._default_renderer
end

--- Push a raw (non-catalog) message directly.
-- @param level "info"|"success"|"warning"|"error"
-- @param text  Already-formatted message text.
function M.raw(level, text)
  local entry = {level = level, text = text, time = os.time()}
  M._queue[#M._queue + 1] = entry
  while #M._queue > M.max_queue do table.remove(M._queue, 1) end
  M.renderer(level, text)
  return entry
end

--- Push a catalog message by id, formatted with the given arguments.
-- @param id  Key in notifications.messages.catalog.
local function emit(id, ...)
  local level, text = messages.format(id, ...)
  return M.raw(level, text)
end

--- Convenience wrappers matching the catalog's own level, but callable
-- under any name — useful for ad-hoc info/success/warning/error text
-- that doesn't warrant a catalog entry.
function M.info(id, ...) return emit(id, ...) end
function M.success(id, ...) return emit(id, ...) end
function M.warning(id, ...) return emit(id, ...) end
function M.error(id, ...) return emit(id, ...) end

--- Return the current queue (oldest first). Read-only convenience for
-- a future UI to drain/display.
function M.queue()
  return M._queue
end

--- Clear all queued notifications without rendering them.
function M.clear()
  M._queue = {}
end

return M
