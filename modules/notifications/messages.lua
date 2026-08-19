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
-- template uses string.format placeholders (%s, %d, ...). Grouped by the
-- same categories as the command registry (modules/commands/registry.lua):
-- File, Edit, View, Buffer, Session/Config, Clipboard, Git, LSP, DAP.
M.catalog = {
  -- File
  file_saved               = {level = 'success', template = 'Saved %s'},
  file_save_failed         = {level = 'error',   template = 'Failed to save %s: %s'},
  file_opened              = {level = 'info',    template = 'Opened %s'},
  file_closed              = {level = 'info',    template = 'Closed %s'},
  file_created             = {level = 'info',    template = 'Created %s'},
  file_deleted             = {level = 'warning', template = 'Deleted %s'},
  file_renamed             = {level = 'info',    template = 'Renamed %s to %s'},
  file_read_only           = {level = 'warning', template = '%s is read-only'},
  file_encoding_changed    = {level = 'info',    template = 'Encoding changed to %s'},
  file_line_ending_changed = {level = 'info',    template = 'Line endings changed to %s'},
  large_file_warning       = {level = 'warning', template = '%s is large — syntax highlighting disabled'},

  -- Edit (includes Search/Tools, folded into Edit like the command registry does)
  buffer_reverted            = {level = 'info',    template = 'Reverted %s to disk'},
  buffer_modified_externally = {level = 'warning', template = '%s was modified externally'},
  search_no_matches          = {level = 'warning', template = 'No matches for "%s"'},
  search_wrapped             = {level = 'info',    template = 'Search wrapped to top'},
  command_unknown            = {level = 'error',   template = 'Unknown command: %s'},
  command_cancelled          = {level = 'info',    template = 'Command cancelled'},
  undo_limit_reached         = {level = 'warning', template = 'Undo history limit reached'},
  find_replace_done          = {level = 'success', template = '%d occurrence(s) replaced'},
  format_applied             = {level = 'success', template = 'Formatting applied'},
  snippet_expanded           = {level = 'info',    template = 'Snippet expanded: %s'},
  multiple_cursors_added     = {level = 'info',    template = '%d cursors added'},
  multiple_cursors_cleared   = {level = 'info',    template = 'Multiple cursors cleared'},
  macro_recording_started    = {level = 'info',    template = 'Recording macro %s'},
  macro_recording_stopped    = {level = 'info',    template = 'Macro %s recorded'},
  macro_played               = {level = 'info',    template = 'Macro %s played'},

  -- View (includes Help, folded into View like the command registry does)
  fold_all      = {level = 'info', template = 'All folds collapsed'},
  unfold_all    = {level = 'info', template = 'All folds expanded'},
  split_created = {level = 'info', template = 'Split created'},
  split_closed  = {level = 'info', template = 'Split closed'},
  theme_changed = {level = 'info', template = 'Theme changed to %s'},
  zoom_changed  = {level = 'info', template = 'Zoom set to %d%%'},

  -- Buffer
  unsaved_changes_warning = {level = 'warning', template = '%s has unsaved changes'},
  buffer_pinned           = {level = 'info',    template = 'Pinned %s'},
  buffer_unpinned         = {level = 'info',    template = 'Unpinned %s'},
  all_buffers_saved       = {level = 'success', template = 'All buffers saved (%d)'},

  -- Session / Config (no matching command-registry category)
  session_saved      = {level = 'success', template = 'Session saved'},
  session_restored   = {level = 'success', template = 'Session restored'},
  layout_reset       = {level = 'info',    template = 'Layout reset to default'},
  workspace_changed  = {level = 'info',    template = 'Workspace changed to %s'},
  config_reloaded    = {level = 'success', template = 'Configuration reloaded'},
  config_error       = {level = 'error',   template = 'Configuration error at line %d: %s'},
  plugin_loaded      = {level = 'info',  template = 'Plugin loaded: %s'},
  plugin_load_failed = {level = 'error', template = 'Plugin failed to load: %s — %s'},

  -- Clipboard
  copied                   = {level = 'info', template = 'Copied'},
  cut                      = {level = 'info', template = 'Cut'},
  pasted                   = {level = 'info', template = 'Pasted'},
  selection_cleared        = {level = 'info', template = 'Selection cleared'},
  paste_replaced_selection = {level = 'info', template = 'Selection replaced'},
  yank_history_cleared     = {level = 'info', template = 'Yank history cleared'},

  -- Git
  git_staged          = {level = 'info',    template = 'Staged %s'},
  git_unstaged        = {level = 'info',    template = 'Unstaged %s'},
  git_committed       = {level = 'success', template = 'Committed: %s'},
  git_push_started    = {level = 'info',    template = 'Pushing to %s...'},
  git_push_ok         = {level = 'success', template = 'Pushed to %s'},
  git_push_failed     = {level = 'error',   template = 'Push to %s failed: %s'},
  git_pull_started    = {level = 'info',    template = 'Pulling from %s...'},
  git_pull_ok         = {level = 'success', template = 'Pulled from %s'},
  git_pull_failed     = {level = 'error',   template = 'Pull from %s failed: %s'},
  git_stash_saved     = {level = 'success', template = 'Stashed changes'},
  git_stash_popped    = {level = 'success', template = 'Stash applied'},
  git_branch_switched = {level = 'info',    template = 'Switched to branch %s'},
  git_merge_conflict  = {level = 'error',   template = 'Merge conflict in %s'},
  git_no_changes      = {level = 'warning', template = 'Nothing to commit'},

  -- LSP (subsystem not implemented yet — ids only, ready for future wiring)
  lsp_server_started      = {level = 'success', template = 'LSP server started: %s'},
  lsp_server_stopped      = {level = 'info',    template = 'LSP server stopped: %s'},
  lsp_server_crashed      = {level = 'error',   template = 'LSP server crashed: %s (%s)'},
  lsp_diagnostics_updated = {level = 'info',    template = '%d diagnostic(s) in %s'},
  lsp_no_definition_found = {level = 'warning', template = 'No definition found'},
  lsp_rename_applied      = {level = 'success', template = 'Renamed %s to %s across %d file(s)'},
  lsp_code_action_applied = {level = 'success', template = 'Applied: %s'},
  lsp_formatting_failed   = {level = 'error',   template = 'LSP formatting failed: %s'},
  lsp_hover_unavailable   = {level = 'warning', template = 'No hover information available'},

  -- DAP (subsystem not implemented yet — ids only, ready for future wiring)
  dap_session_started    = {level = 'info',    template = 'Debug session started'},
  dap_session_stopped    = {level = 'info',    template = 'Debug session stopped'},
  dap_breakpoint_set     = {level = 'info',    template = 'Breakpoint set at %s:%d'},
  dap_breakpoint_removed = {level = 'info',    template = 'Breakpoint removed at %s:%d'},
  dap_breakpoint_invalid = {level = 'warning', template = 'No executable code at %s:%d'},
  dap_program_exited     = {level = 'info',    template = 'Program exited with code %d'},
  dap_attach_failed      = {level = 'error',   template = 'Failed to attach debugger: %s'},
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
