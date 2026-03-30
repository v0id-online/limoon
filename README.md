# Li Moon

**A terminal IDE built on Scintilla and notcurses**

![Build](https://github.com/yuriharrison1/limoon/actions/workflows/build.yml/badge.svg)

---

## What is Li Moon?

Li Moon is a terminal-first IDE forked from [Textadept](https://github.com/orbitalquark/textadept),
replacing the traditional curses backend with [notcurses](https://github.com/dankamongmen/notcurses)
for rich terminal rendering. It uses the [Scintilla](https://www.scintilla.org/) editing component
(via scinterm-notcurses) and is scripted entirely in Lua — every keybinding, menu, and behaviour
is configurable from `~/.limoon/init.lua`.

Li Moon is in **active development / alpha**. Core editing works; LSP, DAP, MCP, libgit2, and
Tree-sitter integration are planned.

---

## Dependencies

| Dependency | Version | Notes |
|---|---|---|
| [notcurses](https://github.com/dankamongmen/notcurses) | >= 3.0.16 | Terminal rendering backend |
| Lua | 5.4 | Scripting engine (fetched by CMake) |
| xsel or wl-clipboard | — | Optional: system clipboard integration |
| libgit2 | — | Planned |
| Tree-sitter | — | Planned |

All other dependencies (Scintilla, LPeg, LFS, CDK, termkey, reproc) are fetched automatically
by CMake via `FetchContent`.

---

## Build

```sh
git clone https://github.com/yuriharrison1/limoon
cd limoon
mkdir build && cd build
cmake ..
make
```

> **Note:** notcurses must be installed system-wide before building.
> On Fedora: `sudo dnf install notcurses-devel`
> On Ubuntu/Debian: `sudo apt install libnotcurses-dev`

---

## Run

```sh
./li
./li path/to/file.lua
```

---

## Configuration

Li Moon is configured in Lua. On first run it creates `~/.limoon/` for user data.

**User init file:** `~/.limoon/init.lua`

Example:

```lua
-- Set a theme
view:set_theme('gruvbox-dark')

-- Map a key
keys['ctrl+shift+t'] = function() limoon.session.save() end
```

Themes live in `themes/`. All built-in modules are in `modules/limoon/`.

---

## License

Li Moon is released under the **BSD 3 License**. 

Li Moon is copyright Yuri Harrison and contributors.
notcurses is copyright Nick Black and contributors.

---

## Status

| Feature | Status |
|---|---|
| Terminal rendering (notcurses) | Working |
| Scintilla editing | Working |
| Lua scripting | Working |
| Themes | Working (33 themes) |
| LSP | Planned |
| DAP | Planned |
| Tree-sitter | Planned |
| libgit2 | Planned |
| MCP | Planned |

---

## GitHub Topics

`terminal` `ide` `lua` `notcurses` `scintilla` `linux` `terminal-ide` `c`
