# Command registry

`modules/commands/registry.lua` is the single source of truth for the
Ctrl+P command palette (`limoon.menu.select_command`, wired to Ctrl+P via
`modules/limoon/keys.lua`). Every command shown in the palette comes from
here — nothing is hand-listed in the palette code itself.

## How it fits together

- `modules/commands/registry.lua` — the API: `register`, `unregister`,
  `get`, `list([category])`, `categories()`.
- `modules/commands/bootstrap.lua` — registers every command from Li
  Moon's built-in menubar (`modules/limoon/menu.lua`'s `default_menubar`)
  automatically, once `events.INITIALIZED` fires. It does not duplicate
  the menu by hand — it walks the real menu table, so the menu and the
  palette can't drift apart.
- Built-in plugins (`plugins/git_status.lua`, `word_count.lua`,
  `scratch_pad.lua`, `help.lua`) self-register their own command via
  `commands.register{...}` next to their existing `keys[...]` binding.
- `modules/limoon/menu.lua`'s `M.select_command()` builds the Ctrl+P list
  from `limoon.commands.list()`/`categories()` — it has no knowledge of
  individual commands.

## Registering a new command

```lua
local commands = require('commands.registry') -- or the `limoon.commands` global, once loaded

commands.register{
  id                 = 'file.save',
  name               = 'Save',
  description        = 'Save current buffer to disk', -- optional
  category           = 'File',
  keybinding         = 'ctrl+s',    -- optional, display-only in the palette
  accelerator_letter = 'S',         -- optional, not yet used by the palette UI
  action             = function() buffer:save() end,
}
```

Required fields: `id` (unique), `name`, `category` (must be one of the
fixed categories below), `action` (a function taking no arguments).

**Rule:** every new user-invokable command must be registered here. If it
isn't, it won't show up in Ctrl+P.

## Categories

The category set is fixed — there is no API to add new ones:

| Category | Covers |
|---|---|
| File | Open/save/close/quit, sessions, workspaces |
| Edit | Text editing, selection, search/replace, snippets, macros, bookmarks, run/build/test, scratch pad, help, word count |
| View | Splits, folding, zoom, whitespace, theme/UI-color pickers, manual/about |
| Buffer | Per-buffer settings: indentation, EOL mode, encoding, lexer, tab bar |
| Window | OS-level window management (currently only populated on the Qt/macOS build) |
| Git | Git integration (currently: git status panel) |
| LSP | Language Server Protocol commands (none yet — planned, see README) |
| DAP | Debug Adapter Protocol commands (none yet — planned, see README) |

Li Moon's existing menubar has `Search`, `Tools`, and `Help` sections that
don't map cleanly onto this set. `modules/commands/bootstrap.lua` folds
them in rather than inventing new categories:

- **Search** → **Edit** (find/replace is a text-editing operation)
- **Tools** → **Edit** (command entry, run/build, bookmarks, macros, and
  snippets are editing-productivity utilities; none of them is really
  "Window", "Git", "LSP", or "DAP")
- **Help** → **View** (informational/display commands)

If you're adding a genuinely new kind of command that doesn't fit any of
the eight categories, pick the closest one and leave a comment — do not
add a ninth category without updating `registry.lua`'s `M.CATEGORIES`
project-wide.

## Known gaps (as of this pass)

- `accelerator_letter` is defined in the command shape but not yet
  rendered or acted on by the palette UI (`ui.dialogs.list` shows a plain
  two-column list; there's no underline/jump-to-letter behavior). Every
  auto-registered built-in command has `accelerator_letter = nil` — the
  source menubar never used `_`/`&` markup, so there's nothing to derive
  one from.
- The palette does not fuzzy-filter as you type beyond whatever
  `ui.dialogs.list`'s own keyboard navigation already provides — building
  real fuzzy search is a separate, larger feature (see the Fuzzy File
  Finder item in `FEATURES_ROADMAP.md`), not attempted here.
- `commands.list()` order is registration order, not alphabetical —
  built-ins register in their curated menu order, which is what an
  existing test (`modules/limoon/menu_test.lua`) already assumes for
  "the first item in the palette is File > New".
