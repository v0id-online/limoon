-- glow_preview.lua — Render a markdown buffer with `glow` in a read-only tab.
--
-- Spawns the `glow` CLI (https://github.com/charmbracelet/glow) against the
-- current buffer's file and shows the rendered output in a new, independent
-- buffer — a snapshot, not a live view of the source. Registered in the
-- command palette as "Preview Markdown (glow)" (see
-- modules/commands/bootstrap.lua).
--
-- Usage:
--   local glow_preview = require('buffer.glow_preview')
--   glow_preview.run(buffer)

local notify = require('notifications.bar')
local ansi_style = require('buffer.ansi_style')

local M = {}

--- Seconds to wait for `glow` before killing it and reporting a failure.
-- Belt-and-suspenders safety net for any hang unrelated to the stdin
-- issue below: without this, a hung `glow` would leave the palette
-- command looking like it silently did nothing forever — no tab, no
-- error, no freeze either (the async spawn doesn't block the UI), just
-- permanent silence.
M.timeout_secs = 15

--- Cached PATH lookup for `glow` (nil = not checked yet this session).
local _glow_available = nil

--- Escape *s* for embedding as one double-quoted token in an os.spawn
-- command line. Li Moon's own argv parser (src/n_limoon.c's parse_cmd)
-- treats a backslash before any character as an escape for that character,
-- so this only needs to protect quotes and backslashes themselves.
-- @param s String to quote.
-- @return quoted string, safe to splice into a command line.
local function quote_arg(s)
  return '"' .. s:gsub('([\\"])', '\\%1') .. '"'
end

--- Check whether `glow` is on PATH. Cached for the session.
-- @return boolean
local function glow_on_path()
  if _glow_available == nil then
    _glow_available = os.execute('command -v glow >/dev/null 2>&1') == true
  end
  return _glow_available
end

--- Check whether *buf* is a markdown file backed by a real path.
-- @param buf Scintilla buffer object.
-- @return boolean
local function is_markdown_buffer(buf)
  local path = buf and buf.filename
  if not path then return false end
  local ext = path:match('%.([%w]+)$')
  return ext == 'md' or ext == 'markdown'
end

--- Render *buf*'s file with `glow -s dark` and show the result in a new,
-- read-only buffer. No-op (with a notification) if *buf* isn't a markdown
-- file on disk, or if `glow` isn't installed, or if `glow` itself fails.
--
-- Fully asynchronous: uses os.spawn's stdout_cb/stderr_cb/exit_cb rather
-- than proc:read()/proc:wait(). A synchronous proc:read('a') only polls
-- the stdout fd (src/n_limoon.c's poll_with_ui, called from
-- read_process_output) — it never drains stderr, which is instead only
-- ever drained by monitor_processes() on the main event-loop tick. Doing
-- a blocking stdout read while also expecting a stderr_cb to fire
-- deadlocks the moment `glow` writes enough to stderr to fill that pipe:
-- `glow` blocks writing to a full, undrained stderr pipe, and Li Moon
-- blocks reading stdout that will now never arrive. The callback-driven
-- form here is the same non-blocking pattern modules/limoon/run.lua uses.
--
-- The command is `sh -c 'exec glow -s dark "$1" </dev/null' sh <path>`,
-- not a direct `glow -s dark <path>`. os.spawn always gives the child an
-- open pipe for stdin. `glow` (like most cobra-based CLIs) treats "stdin
-- is a FIFO/pipe" as "input is being piped to me" and reads THAT instead
-- of the file argument — regardless of whether the pipe has data or is
-- already at EOF, so closing our end with proc:close() only fixed the
-- hang (glow no longer blocks reading it) while leaving the render empty
-- (glow rendered zero-byte piped stdin instead of the file — confirmed:
-- direct spawn produced a 2-byte output for two different real files).
-- A pipe closed by proc:close() is still a pipe. A real /dev/null is a
-- character device, which is the specific type `glow` treats as "no
-- piped input, use the file argument" — os.spawn has no way to give the
-- child that directly, so `sh -c ... </dev/null` does it instead. The
-- path is passed as sh's $1 (positional parameter), not interpolated
-- into the script text, so it needs no shell-metacharacter escaping.
-- @param buf Scintilla buffer object to preview (typically the current
--   `buffer` global).
function M.run(buf)
  if not is_markdown_buffer(buf) then
    notify.warning('glow_preview_invalid_buffer')
    return
  end

  if not glow_on_path() then
    notify.error('glow_not_found')
    return
  end

  local path = buf.filename
  local basename = path:match('([^/\\]+)$') or path
  local stdout_chunks, stderr_chunks = {}, {}
  local finished = false -- set by whichever of exit_cb / the watchdog reports first

  local cmd = 'sh -c ' .. quote_arg('exec glow -s dark "$1" </dev/null') .. ' sh ' .. quote_arg(path)
  local proc = os.spawn(cmd,
    -- stdout_cb: raw chunks, ANSI escapes and all — buffer.ansi_style
    -- strips/converts them once the full output is in hand (see below).
    function(chunk) stdout_chunks[#stdout_chunks + 1] = chunk end,
    function(chunk) stderr_chunks[#stderr_chunks + 1] = chunk end,
    function(status)
      if finished then return end -- already reported by the watchdog timeout
      finished = true
      local rendered = table.concat(stdout_chunks)

      if status ~= 0 then
        local err_text = table.concat(stderr_chunks):match('^%s*(.-)%s*$')
        if err_text ~= '' and #err_text <= 200 then
          notify.error('glow_render_failed', err_text)
        else
          notify.error('glow_render_failed', 'exit code ' .. tostring(status))
        end
        return
      end

      local plain, spans = ansi_style.parse(rendered)

      local new_buf = buffer.new()
      new_buf._type = '[glow] ' .. basename
      new_buf.read_only = false
      new_buf:append_text(plain)
      ansi_style.apply(new_buf, view, spans)
      new_buf:set_save_point()
      new_buf.read_only = true
    end)

  if not proc then
    notify.error('glow_render_failed', 'failed to spawn glow')
    return
  end

  -- We never write anything to the spawned `sh`'s stdin (it doesn't read
  -- it either — glow's own stdin is separately redirected from /dev/null
  -- by the command line above). Close it so nothing is left holding a
  -- pipe open for no reason.
  proc:close()

  timeout(M.timeout_secs, function()
    if not finished and proc:status() == 'running' then
      finished = true
      proc:kill()
      notify.error('glow_render_failed', 'timed out after ' .. M.timeout_secs .. 's')
    end
    return false -- one-shot, never repeat
  end)
end

return M
