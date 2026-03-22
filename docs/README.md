# 🌙 Li Moon

> **A terminal IDE that doesn't apologize for living in the terminal.**

```
 ██╗     ██╗    ███╗   ███╗ ██████╗  ██████╗ ███╗   ██╗
 ██║     ██║    ████╗ ████║██╔═══██╗██╔═══██╗████╗  ██║
 ██║     ██║    ██╔████╔██║██║   ██║██║   ██║██╔██╗ ██║
 ██║     ██║    ██║╚██╔╝██║██║   ██║██║   ██║██║╚██╗██║
 ███████╗██║    ██║ ╚═╝ ██║╚██████╔╝╚██████╔╝██║ ╚████║
 ╚══════╝╚═╝    ╚═╝     ╚═╝ ╚═════╝  ╚═════╝ ╚═╝  ╚═══╝
```

[![Build](https://github.com/yuriharrison1/limoon/actions/workflows/build.yml/badge.svg)](https://github.com/yuriharrison1/limoon/actions)
[![License: CR-BSD](https://img.shields.io/badge/license-CR--BSD-blue.svg)](./LICENSE)
[![Platform](https://img.shields.io/badge/platform-Linux-orange.svg)](https://github.com/yuriharrison1/limoon)
[![Status](https://img.shields.io/badge/status-MVP%20%2F%20expanding-green.svg)](https://github.com/yuriharrison1/limoon)

---

## What is Li Moon?

Li Moon is a **terminal-native IDE** built for developers who chose the terminal
not as a workaround, but as an architecture decision.

It is not a Vim clone. It is not a TUI wrapper around VS Code concepts.
It is a ground-up rethink of what a serious development environment looks like
when you remove the assumption that a GUI is required.

Built on **Scintilla** (the same battle-tested editor component behind Notepad++
and many others) and **notcurses** (true 24-bit RGB, Unicode, and GPU-accelerated
terminal rendering), Li Moon delivers editor-grade text handling with a rendering
layer that actually respects what modern terminals can do.

All behavior is scriptable in **Lua** — not as an afterthought, but as the
primary interface between you and the editor.

---

## Why Li Moon?

Most terminal editors fall into one of two traps:

- **Modal editors** (Vim, Neovim, Helix) — powerful, but their identity is
  the modal paradigm. Everything bends to it.
- **TUI wrappers** — take a GUI editor, slap a terminal renderer on it,
  and call it a day.

Li Moon is neither. It is a **terminal-first IDE** in the same sense that
Emacs is an editor-first OS — the terminal is not a constraint, it is the
design target.

---

## Architecture

```
┌─────────────────────────────────────────────┐
│                  Lua Layer                  │  ← All configuration, plugins,
│             (init.lua, modules)             │     keybindings, behavior
├─────────────────────────────────────────────┤
│               Li Moon Core (C)              │  ← Event loop, window manager,
│            src/n_limoon.c                   │     Lua bridge, lifecycle
├───────────────────┬─────────────────────────┤
│   Scintilla       │   scinterm-notcurses     │  ← Editor engine + notcurses
│   (editor core)   │   (rendering backend)    │     terminal rendering
├───────────────────┴─────────────────────────┤
│                 notcurses                   │  ← 24-bit RGB, Unicode,
│         (terminal rendering layer)          │     Kitty/Sixel graphics
└─────────────────────────────────────────────┘
```

---

## Current Status

Li Moon is **MVP / actively expanding**. The core editor is functional.
The roadmap below is being executed now.

### ✅ Done
- Full Scintilla editor via notcurses backend (scinterm-notcurses)
- 24-bit RGB rendering with true color support
- Lua scripting throughout
- Tabs with 24-bit RGB gradient rendering
- Syntax highlighting foundation
- Multi-window support

### 🔧 In Progress
- Arena allocator for render hot path (zero malloc/free per frame)
- Dirty region tracking (skip unchanged plane renders)
- Event loop optimization (input batching + 60fps frame throttle)
- Kitty / Sixel graphics protocol detection and override

### 📋 Roadmap
| Feature | Status |
|---|---|
| LSP (Language Server Protocol) | Planned |
| DAP (Debug Adapter Protocol) | Planned |
| MCP (Model Context Protocol) | Planned |
| Treesitter incremental parsing | Planned |
| libgit2 integration | Planned |
| File tree / fuzzy finder | Planned |
| Terminal multiplexer built-in | Planned |

---

## Dependencies

| Dependency | Version | Purpose |
|---|---|---|
| [notcurses](https://github.com/dankamongmen/notcurses) | >= 3.0.16 | Terminal rendering |
| Lua | 5.4 | Scripting engine |
| CMake | >= 3.20 | Build system |
| libgit2 | planned | Git integration |
| tree-sitter | planned | Incremental parsing |

### Fedora / RHEL

```bash
sudo dnf install notcurses-devel lua-devel cmake gcc gcc-c++
```

### Debian / Ubuntu

```bash
sudo apt install libnotcurses-dev liblua5.4-dev cmake gcc g++
```

---

## Build

```bash
git clone https://github.com/yuriharrison1/limoon.git
cd limoon
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
```

---

## Run

```bash
./li
# or with a file
./li path/to/file.lua
```

---

## Configuration

Li Moon is configured entirely in Lua.

```bash
mkdir -p ~/.config/limoon
$EDITOR ~/.config/limoon/init.lua
```

```lua
-- ~/.config/limoon/init.lua

-- Theme
li.theme = "catppuccin-mocha"

-- Tabs
li.tabs = {
  gradient = true,
  gradient_start = 0x89b4fa,  -- blue
  gradient_end   = 0xcba6f7,  -- mauve
}

-- Graphics protocol override (auto-detected by default)
li.graphics = {
  protocol = "auto",  -- "kitty", "sixel", "auto", "none"
}

-- Keybindings
keys["ctrl+p"] = function() li.find_file() end
keys["ctrl+b"] = function() li.toggle_tree() end
```

---

## Themes

Li Moon ships with a rich set of themes out of the box:

```
catppuccin-mocha  catppuccin-latte  dracula       nord
tokyo-night       one-dark          gruvbox-dark  kanagawa
rose-pine         everforest-dark   ayu-dark      monokai
solarized-dark    solarized-light   github-dark   phosphor
```

---

## Design Philosophy

**1. The terminal is not a limitation.**
notcurses gives us true 24-bit color, Unicode, and graphics protocols.
Li Moon uses all of it.

**2. C for performance, Lua for everything else.**
The core is lean, fast C. All behavior that can move to Lua, does.
No unnecessary complexity in the binary.

**3. Everything builtin, nothing mandatory.**
LSP, DAP, MCP, git — all built-in C modules with no external process
dependencies beyond the language servers themselves.

**4. Unix lineage.**
Li Moon respects the Unix philosophy. It does its job, composes well,
and gets out of your way.

---

## License

Li Moon is licensed under the **CR-BSD License** (Community Reciprocity BSD).

A BSD 3-Clause variant with an ethical commercial notification pact and
a 180-day contribution window.

> *"Pegue, use, lucre — mas lembre de onde veio."*

License MIT

---

## Contributing

Li Moon is in active development. Contributions welcome.

Before contributing, read the coding standards:
- All code and comments in **English**
- Doxygen documentation on all functions
- No malloc/free in render hot path
- Tests for all new modules

---

## Author

Built by [Yuri Harrison](https://github.com/yuriharrison1) —
maker, engineer, terminal maximalist, Fortaleza/CE 🇧🇷

---

*Li Moon — because the terminal deserves better.*
