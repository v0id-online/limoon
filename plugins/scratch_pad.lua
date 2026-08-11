-- Plugin: Scratch Pad
-- A persistent notes buffer that auto-saves to ~/.limoon/scratch.txt
-- Ctrl+Shift+N  → open / focus / toggle
-- Ctrl+S inside → save (handled by standard file save once _type cleared)

local W = require('ui_widgets')

local SCRATCH_FILE = _USERHOME .. '/scratch.txt'
local SCRATCH_TYPE = '[Scratch Pad]'
local _buf = nil   -- cached buffer reference

local function find_scratch()
  for _, b in ipairs(_BUFFERS) do
    if b._type == SCRATCH_TYPE then return b end
  end
  return nil
end

local function open_scratch()
  local existing = find_scratch()

  -- Toggle: if the scratch buffer is already active, close it.
  if existing and _G.buffer == existing then
    -- Switch back to the previously visited buffer instead of closing.
    view:goto_buffer(-1)
    return
  end

  if existing then
    view:goto_buffer(existing)
    return
  end

  -- Load from file if it exists, otherwise create a fresh buffer.
  if lfs.attributes(SCRATCH_FILE) then
    io.open_file(SCRATCH_FILE)
    _buf = buffer
    _buf._type = SCRATCH_TYPE
  else
    _buf = buffer.new()
    _buf._type = SCRATCH_TYPE
    _buf:append_text('-- Scratch Pad --\n\n')
    _buf:set_save_point()
  end
  _buf:set_lexer('text')
  W.notify('Scratch Pad  (auto-saves on close)', 3)
end

local function save_scratch(buf)
  buf = buf or find_scratch()
  if not buf then return end

  -- Write atomically via .tmp + rename to avoid data loss on crash, and
  -- surface every failure instead of silently discarding the buffer's
  -- content (io.open/write/close/rename can all fail: disk full,
  -- permission denied, read-only filesystem).
  local tmp_path = SCRATCH_FILE .. '.tmp'
  local f, err = io.open(tmp_path, 'w')
  if not f then
    W.notify('Scratch pad save FAILED (open): ' .. tostring(err), 5)
    return
  end

  local ok, werr = f:write(buf:get_text())
  if not ok then
    W.notify('Scratch pad save FAILED (write): ' .. tostring(werr), 5)
    f:close()
    os.remove(tmp_path)
    return
  end

  local ok_close, cerr = f:close()
  if not ok_close then
    W.notify('Scratch pad save FAILED (close): ' .. tostring(cerr), 5)
    os.remove(tmp_path)
    return
  end

  -- Atomic swap: only replace SCRATCH_FILE if the write succeeded fully.
  local ok_ren, rerr = os.rename(tmp_path, SCRATCH_FILE)
  if not ok_ren then
    W.notify('Scratch pad save FAILED (rename): ' .. tostring(rerr), 5)
    os.remove(tmp_path)
    return
  end

  buf:set_save_point()
end

-- Auto-save when leaving the scratch buffer.
events.connect(events.BUFFER_BEFORE_SWITCH, function()
  if buffer._type == SCRATCH_TYPE then save_scratch(buffer) end
end)

-- Auto-save when quitting.
events.connect(events.QUIT, function() save_scratch() end)

keys['ctrl+shift+n'] = open_scratch

return {
  _meta = {
    name        = 'Scratch Pad',
    description = 'Quick notes buffer (Ctrl+Shift+N), auto-saves to scratch.txt',
    version     = '1.0',
    author      = 'built-in',
  }
}
