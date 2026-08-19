# Notification catalog and bar

Foundation modules for a centralized notification system:
`modules/notifications/messages.lua` (the catalog) and
`modules/notifications/bar.lua` (the queue + renderer).

> **Status:** foundation only. Nothing in Li Moon currently calls into
> this module. `modules/ui_widgets.lua`'s existing `M.notify(text, secs)`
> statusbar notification is untouched and still the active code path.
> Wiring subsystems over to `notifications.bar` — and enforcing it as the
> only path (banning ad-hoc `ui.statusbar_text` writes and `notify.raw`
> outside a deliberate escape hatch) — is deferred until after the
> current bugfix cycle.

## Adding a new message id

Edit `modules/notifications/messages.lua` and add an entry to
`M.catalog`:

```lua
M.catalog.my_new_id = {level = 'info', template = 'Something happened: %s'}
```

- `level` must be one of `info`, `success`, `warning`, `error`.
- `template` is a `string.format` template — use `%s`/`%d`/etc. for
  interpolated values.

Then anywhere in the codebase:

```lua
local notify = require('notifications.bar')
notify.info('my_new_id', some_value)
```

`notify.info/success/warning/error` are convenience wrappers around the
same lookup — the level actually shown is always the one recorded in the
catalog entry, not implied by which wrapper you call.

## Catalog coverage

The catalog is grouped by the same categories as the command registry
(File, Edit, View, Buffer, Clipboard, Git) plus Session/Config, LSP, and
DAP. **LSP and DAP ids are pre-seeded for future subsystems; do not wire
them into anything until those features actually exist** — they exist
now so LSP/DAP work can call `notify.*` from day one instead of adding
catalog entries piecemeal later.

## Level conventions

| Level | Meaning | Default duration |
|---|---|---|
| `info` | Neutral, expected event (file opened, command run) | 2s |
| `success` | An action the user asked for completed (save, push) | 2s |
| `warning` | Something the user should notice but isn't broken (no matches, external modification) | 4s |
| `error` | An action failed or a hard error occurred | 6s |

Durations live in `bar.M.durations` and are keyed by level, not by id —
changing an id's severity automatically changes how long it's shown.

## Ad-hoc messages

For text that doesn't warrant a catalog entry (rare — prefer adding an
id), use the escape hatch:

```lua
notify.raw('warning', 'ad-hoc message text')
```

This bypasses the catalog but still goes through the same queue and
renderer, so it behaves identically to a catalog-backed notification.

## Rendering

`bar.lua` ships with a stub renderer (`io.stderr:write`) so the module is
usable standing alone. The real UI integration — a notcurses plane
rendering an overlay/toast in the terminal — is marked
`TODO(yuri): wire to actual notification plane` in `bar.lua` next to
`M._default_renderer`. Swap it via:

```lua
notify.set_renderer(function(level, text) ... end)
```

## Enforcement (not yet active)

Once existing subsystems (file I/O, git plugin, LSP, etc.) are migrated
to call `notify.*` instead of writing `ui.statusbar_text` or
`ui_widgets.M.notify` directly, a follow-up task should:

- Ban direct `ui.statusbar_text` writes from subsystem code (statusbar
  segments via `ui_widgets.status_add` are unaffected — that's a
  different, still-valid mechanism).
- Restrict `notify.raw` to genuinely ad-hoc cases, flagged in review.

This is explicitly **out of scope** for this task — no existing call
site has been changed.
