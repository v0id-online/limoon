# textadept-notcurses

> A drop-in third binary for [Textadept](https://orbitalquark.github.io/textadept/)
> that replaces the curses backend with [notcurses](https://github.com/dankamongmen/notcurses) —
> bringing true 24-bit color, smooth rendering, and a modern terminal UI to one of the
> most elegant editors ever written.

---

## Why

Textadept is fast, minimal, and scriptable to the bone. Its terminal binary (`textadept-curses`)
is solid — but curses is a 1980s library carrying 1980s constraints. No true color. No compositing.
No double-buffered rendering. Flickery redraws on large files.

notcurses was built for modern terminals from scratch. Swapping the backend unlocks a completely
different class of UI — without changing a single line of Lua, without touching the editor's core,
and without sacrificing startup speed or resource efficiency.

This project is that swap.

---

## What's different

- **Tabs with 24-bit gradient colors and × close buttons**
  Not a theme trick — actual RGB gradients rendered natively in the terminal.

- **Richer UI throughout**
  Borders, panels, and the status bar all benefit from notcurses' compositing model.
  The editor looks noticeably better with zero additional CPU cost.

- **Double-buffered rendering**
  Eliminates flicker on large buffer redraws. Feels faster because it is.

- **Built-in help overlay**
  Press `F1` for a contextual help panel rendered with notcurses widgets.

- **Demo themes and plugins**
  A curated set included out of the box — both as a usable starting point and as
  documented examples of what the new backend makes possible.

- **Developer documentation**
  A dedicated doc tree covering the notcurses integration, the widget model, and
  how to write plugins that take advantage of the new API.

---

## Install
```sh
sh <(curl -sSf https://raw.githubusercontent.com/yuriharrison1/textadept/default/install.sh)
```

Or build from source — see [BUILDING.md](./BUILDING.md).

---

## Compatibility

Existing Textadept plugins and themes **continue to work unchanged**. The Lua API surface
is identical to the standard terminal binary. Plugins that want to use notcurses-specific
features (gradients, blitters, pixel graphics) can opt in through a small extension API
documented in [docs/notcurses-api.md](./docs/notcurses-api.md).

The notcurses library is widely packaged:

| Distro | Install |
|---|---|
| Fedora | `dnf install notcurses` |
| Debian / Ubuntu | `apt install libnotcurses-dev` |
| Arch | `pacman -S notcurses` |
| macOS | `brew install notcurses` |

---

## What notcurses unlocks

| Capability | curses | notcurses |
|---|---|---|
| Color depth | 256 / 8 | Full 24-bit RGB |
| Unicode & emoji | Limited, fragile | First-class, correct |
| Pixel graphics (Kitty, Sixel) | ✗ | ✓ |
| Compositing / blitter model | ✗ | ✓ |
| Double-buffered rendering | ✗ | ✓ |
| Active development | Stagnant | Active |

---

## Relationship to upstream Textadept

This is **not a fork** intended to live in parallel with Textadept. The goal is to bring
this backend upstream as an official third binary — alongside `textadept` (GUI) and
`textadept-curses` (terminal).

A formal proposal has been submitted to the Textadept maintainer. In the meantime, this
repository is the working implementation.

---

## License

MIT — same as Textadept.

---

## Author

**Yuri Harrison** — [@yuriharrison1](https://github.com/yuriharrison1)
