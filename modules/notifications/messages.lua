-- messages.lua — Central notification message catalog for Li Moon.
--
-- Every user-facing notification should have an id here rather than an
-- inline string at the call site, so wording and severity stay
-- consistent and easy to audit/change in one place.
--
-- Usage:
--   local messages = require('notifications.messages')
--   local level, text = messages.format('file_saved', 'init.lua')

local M = {}

--- id -> { level = "info"|"success"|"warning"|"error", template = string }
-- template uses string.format placeholders (%s, %d, ...).
M.catalog = {
  file_saved                 = {level = 'success', template = 'Saved %s'},
  file_save_failed           = {level = 'error',   template = 'Failed to save %s: %s'},
  file_opened                = {level = 'info',    template = 'Opened %s'},
  file_closed                = {level = 'info',    template = 'Closed %s'},
  buffer_reverted            = {level = 'info',    template = 'Reverted %s to disk'},
  buffer_modified_externally = {level = 'warning', template = '%s was modified externally'},

  copied                     = {level = 'info',    template = 'Copied'},
  cut                        = {level = 'info',    template = 'Cut'},
  pasted                     = {level = 'info',    template = 'Pasted'},
  selection_cleared          = {level = 'info',    template = 'Selection cleared'},

  search_no_matches          = {level = 'warning', template = 'No matches for "%s"'},
  search_wrapped             = {level = 'info',    template = 'Search wrapped to top'},

  command_unknown            = {level = 'error',   template = 'Unknown command: %s'},
  command_cancelled          = {level = 'info',    template = 'Command cancelled'},

  lsp_server_started         = {level = 'success', template = 'LSP server started: %s'},
  lsp_server_stopped         = {level = 'info',    template = 'LSP server stopped: %s'},
  lsp_server_crashed         = {level = 'error',   template = 'LSP server crashed: %s (%s)'},

  git_staged                 = {level = 'info',    template = 'Staged %s'},
  git_unstaged               = {level = 'info',    template = 'Unstaged %s'},
  git_committed               = {level = 'success', template = 'Committed: %s'},
  git_push_started           = {level = 'info',    template = 'Pushing to %s...'},
  git_push_ok                = {level = 'success', template = 'Pushed to %s'},
  git_push_failed            = {level = 'error',   template = 'Push to %s failed: %s'},
  git_pull_started           = {level = 'info',    template = 'Pulling from %s...'},
  git_pull_ok                = {level = 'success', template = 'Pulled from %s'},
  git_pull_failed            = {level = 'error',   template = 'Pull from %s failed: %s'},
}

--- Look up an id and format it with the given arguments.
-- @param id    Catalog key (see M.catalog).
-- @param ...   Values substituted into the template via string.format.
-- @return level string, formatted text string
-- Raises an error if id is not in the catalog — this is deliberate: an
-- unregistered notification id is a programming error, not a runtime
-- condition to degrade gracefully from.
function M.format(id, ...)
  local entry = M.catalog[id]
  if not entry then error('notifications.messages: unknown id "' .. tostring(id) .. '"', 2) end
  local ok, text = pcall(string.format, entry.template, ...)
  if not ok then
    error('notifications.messages: bad arguments for id "' .. id .. '": ' .. text, 2)
  end
  return entry.level, text
end

return M
