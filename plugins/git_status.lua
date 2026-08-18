-- Plugin: Git Status
-- Shows the current git branch (and dirty marker *) in the statusbar.
-- Ctrl+Shift+G opens a git status output buffer.

local W = require('ui_widgets')
local commands = require('commands.registry')

-- Cache result for 5 seconds to avoid shelling out on every keypress.
local _cache, _cache_time = '', 0

local function git_branch()
  local now = os.time()
  if now - _cache_time < 5 then return _cache end
  _cache_time = now

  local root = io.get_project_root()
  if not root then _cache = ''; return '' end

  local f = io.popen(string.format('git -C %q rev-parse --abbrev-ref HEAD 2>/dev/null', root))
  if not f then _cache = ''; return '' end
  local branch = f:read('l') or ''
  f:close()

  if branch == '' or branch == 'HEAD' then
    -- Detached head: show short hash
    local fh = io.popen(string.format('git -C %q rev-parse --short HEAD 2>/dev/null', root))
    branch = fh and (fh:read('l') or '') or ''
    if fh then fh:close() end
    branch = branch ~= '' and ('#' .. branch) or ''
  end

  if branch ~= '' then
    local fd = io.popen(string.format(
      'git -C %q status --porcelain 2>/dev/null | head -1', root))
    local dirty = fd and fd:read('l') or ''
    if fd then fd:close() end
    _cache = ' ' .. branch .. (dirty ~= '' and '*' or '')
  else
    _cache = ''
  end
  return _cache
end

W.status_add('git_status', git_branch, 5)

-- Ctrl+Shift+G → show full `git status` in an output buffer
local function show_git_status()
  local root = io.get_project_root()
  if not root then W.notify('Not a git repository', 3); return end

  local f = io.popen(string.format('git -C %q status 2>&1', root))
  local out = f and f:read('a') or '(error)'
  if f then f:close() end

  local buf = W.output_buffer('[Git Status]')
  W.write(buf, out, true)
  buf:set_lexer('bash')
end
keys['ctrl+shift+g'] = show_git_status

commands.register{
  id = 'git.status',
  name = 'Git Status',
  description = 'Show `git status` for the current project in an output buffer',
  category = 'Git',
  keybinding = 'ctrl+shift+g',
  action = show_git_status,
}

return {
  _meta = {
    name        = 'Git Status',
    description = 'Git branch in statusbar + Ctrl+Shift+G status panel',
    version     = '1.0',
    author      = 'built-in',
  }
}
