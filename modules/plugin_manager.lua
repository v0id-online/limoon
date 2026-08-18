-- plugin_manager.lua — Discovers and loads Lua plugins.
--
-- Plugins are .lua files that return a table with an optional _meta field:
--   return {
--     _meta = { name='My Plugin', description='...', version='1.0', author='...' },
--     setup = function() ... end,   -- called once after loading
--   }
--
-- Search order:
--   1. _USERHOME/plugins/   (user plugins — highest priority)
--   2. _HOME/plugins/       (built-in example plugins)

local M = {}

--- Table of loaded plugin records: { meta={}, file='', ok=true/false, err='' }
M.plugins = {}

--- Directories searched for plugins (in order).
M.dirs = {
  _USERHOME .. '/plugins',
  _HOME     .. '/plugins',
}

--- Load a single plugin file.
-- @param file  Absolute path to the .lua file.
-- @return true on success; false, error_message on failure.
function M.load(file)
  local ok, result = pcall(dofile, file)
  local record = {file = file, ok = ok}
  local id
  if ok then
    if type(result) == 'table' then
      record.meta = result._meta or {}
      id = record.meta.name or file:match('([^/\\]+)%.lua$') or file
      -- M.dirs is searched in priority order (_USERHOME before _HOME), so
      -- the first plugin registered under an id is the highest-priority
      -- one; skip re-registering (and running setup() again) if a
      -- lower-priority plugin with the same id loads afterward.
      if M.plugins[id] then return M.plugins[id].ok, M.plugins[id].err end
      M.plugins[id] = record
      -- Call setup() if provided.
      if type(result.setup) == 'function' then
        local ok2, err2 = pcall(result.setup)
        if not ok2 then
          record.ok, record.err = false, err2
        end
      end
    else
      -- Plugin returned nothing (side-effect-only) — register by filename.
      id = file:match('([^/\\]+)%.lua$') or file
      if M.plugins[id] then return M.plugins[id].ok, M.plugins[id].err end
      record.meta = {}
      M.plugins[id] = record
    end
  else
    record.err = result  -- result holds the error message on failure
    id = file:match('([^/\\]+)%.lua$') or file
    if M.plugins[id] then return M.plugins[id].ok, M.plugins[id].err end
    M.plugins[id] = record
  end
  return ok, record.err
end

--- Load all plugins from a single directory.
function M.load_dir(dir)
  local attrs = lfs.attributes(dir)
  if not attrs or attrs.mode ~= 'directory' then return end
  for file in lfs.dir(dir) do
    if file:match('%.lua$') then
      M.load(dir .. '/' .. file)
    end
  end
end

--- Discover and load plugins from all configured directories.
function M.load_all()
  for _, dir in ipairs(M.dirs) do
    M.load_dir(dir)
  end
  local n, errs = 0, 0
  for _, rec in pairs(M.plugins) do
    if rec.ok then n = n + 1 else errs = errs + 1 end
  end
  if errs > 0 then
    events.connect(events.INITIALIZED, function()
      ui.statusbar_text = string.format('Plugins: %d loaded, %d error(s)', n, errs)
    end, 1)
  end
end

--- Show an interactive list of all loaded plugins.
function M.show()
  local items = {}
  for id, rec in pairs(M.plugins) do
    local meta = rec.meta or {}
    local status = rec.ok and 'OK' or 'ERR'
    local name = meta.name or id
    local desc = meta.description or (rec.err and rec.err:match('[^\n]*') or '—')
    local ver  = meta.version or ''
    items[#items + 1] = status
    items[#items + 1] = name
    items[#items + 1] = ver
    items[#items + 1] = desc
  end
  if #items == 0 then
    ui.dialogs.message{title = 'Plugin Manager', text = 'No plugins loaded.'}
    return
  end
  ui.dialogs.list{
    title   = 'Loaded Plugins',
    columns = {'Status', 'Name', 'Version', 'Description'},
    items   = items,
  }
end

return M
