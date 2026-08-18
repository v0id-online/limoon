# Li Moon - TODOs and Future Improvements

> File auto-generated from a project bug sweep.
> Date: 2026-03-28

---

## 🐛 Known Bugs

### Core

#### file_io_test.lua
- **Line 255**: Test skipped on macOS due to a system iconv error
  ```lua
  if OSX then skip('crashes on macOS due to system iconv error') end
  ```

- **Line 717**: `events.BUFFER_DELETED` handler assumes the closed buffer was focused
  ```lua
  expected_failure() -- TODO: ui.lua's events.BUFFER_DELETED handler assumes closed buffer was focused
  ```

#### init_test.lua
- **Line 123**: Incomplete quit test
  ```lua
  -- TODO: quit?
  ```

#### ui_test.lua
- **Line 545**: Problem with `splits[2].size[3] == 0` (only works on GTK2)
  ```lua
  if not gtk2 then expected_failure() end -- TODO: splits[2].size[3] == 0
  ```

- **Line 583**: Expected failure (reason unspecified)
  ```lua
  expected_failure() -- TODO:
  ```

- **Line 595**: Popup menu not implemented in tests
  ```lua
  -- TODO: ui.popup_menu
  ```

#### view_test.lua
- **Line 38**: Expected failure
  ```lua
  expected_failure() -- TODO:
  ```

- **Line 73**: Problem with second `size[3] == 0` (only works on GTK2)
  ```lua
  if not gtk2 then expected_failure() end -- TODO: second size[3] == 0
  ```

#### ui_dialogs_test.lua
- **Line 16**: Dialog tests cause focus issues on macOS
  ```lua
  if OSX then skip('this test appears to cause focus issues for command entry tests') end
  ```

- **Line 31**: Mocking the rest seems pointless
  ```lua
  -- TODO: mocking the rest of these seems pointless.
  ```

---

## ⏳ Unimplemented Features

### Menu/Commands (modules/limoon/keys.lua)

| Line | Feature | Description |
|-------|----------------|-------------|
| 361 | `io.open_recent_file` | Open recent file |
| 362 | `buffer.reload` | Reload buffer |
| 365 | `io.save_all_files` | Save all files |
| 368 | `limoon.sessions.load` | Load session |
| 369 | `limoon.sessions.save` | Save session |
| 413 | `limoon.history.record` | Record navigation history |
| 414 | `limoon.history.clear` | Clear navigation history |
| 420 | `ui.find.find_next` | Find next (via menu) |
| 421 | `ui.find.find_prev` | Find previous (via menu) |
| 422 | `ui.find.replace` | Replace (via menu) |
| 423 | `ui.find.replace_all` | Replace all (via menu) |
| 451 | `limoon.bookmarks.clear` | Clear bookmarks |
| 458 | `limoon.macros.save` | Save recorded macro |
| 459 | `limoon.macros.load` | Load saved macro |
| 463 | `Quick Open Current Directory` | Quickly open the current directory |
| 466 | `limoon.snippets.select` | Select snippet |
| 470 | `Complete Trigger Word` | Complete trigger word |
| 473 | `Tools/Show Style` | Show style under cursor |
| 483-485 | Tab width presets | Predefined tab widths (2, 3, 4, 8) |

### Session (modules/limoon/session.lua)

- **Line 86**: Review `split_pos` - may be incorrect
  ```lua
  if i == 1 then view.split_pos = split.size end -- TODO: split_pos?
  ```

- **Line 172 (test)**: Expected failure
  ```lua
  expected_failure() -- TODO:
  ```

### Snippets (modules/limoon/snippets.lua)

- **Line 673**: Insert initial transform when `ph.index > self.index`
  ```lua
  -- TODO: insert initial transform for ph.index > self.index
  ```

### Run (modules/limoon/run_test.lua)

- **Line 381**: Output lexer does not recognize absolute Windows paths (C:\)
  ```lua
  if WIN32 then expected_failure() end -- TODO: output lexer does not recognize absolute c:\ paths
  ```

### Keys (modules/limoon/keys_test.lua)

- **Line 36**: Test fails randomly
  ```lua
  skip('this test randomly fails') -- TODO: no amount of ui.update() is good enough
  ```

---

## 🏗️ Architectural Limitations

### Split View (C)

**File**: `src/n_limoon.c:1746`

```c
/* ------------------------------------------------------------------ */
/* Split/pane (Bug H — stub: proper pane tree requires significant work) */
```

- The pane tree is a stub implementation
- Requires significant work to function correctly
- Known issues:
  - Pane resizing may not work correctly
  - Focus between split views may be inconsistent
  - Unsplit may not restore the correct layout

### Lexer (C/Lua)

**File**: `core/lexer_test.lua:222`

```lua
-- TODO: test buffer/scintilla <-> lexer api
```

- The buffer/Scintilla <-> lexer API needs more tests

---

## 📝 Documentation Improvements

### Keys (core/keys.lua)

- **Line 48**: Document unrecognized key codes
  ```lua
  -- a trailing "0x*XXXX*", that number can be aliased to a string representation in `keys.KEYSYMS`.
  ```

---

## 🔧 Build/Warning Issues

### C - Warnings (non-critical)

| File | Line | Description |
|---------|-------|-------------|
| `n_limoon.c` | 2099 | Unused return value of `luaL_dostring` |
| `n_limoon.c` | 3468-3507 | Possible truncation in file browser `snprintf` |

---

## 💡 Improvement Suggestions

### 1. Plugin System
- Improve error handling in plugins
- Add plugin hot-reload
- Dependency system between plugins

### 2. Terminal/Notcurses
- Full mouse support
- Terminal resize support (SIGWINCH)
- Native terminal clipboard support

### 3. LSP/DAP (Planned)
- Language Server Protocol integration
- Debug Adapter Protocol
- Tree-sitter for syntax highlighting

### 4. Git
- libgit2 integration (instead of shelling out)
- Inline visual diff
- Blame annotations

### 5. Performance
- Lazy loading of large modules
- Rendering optimization for large files
- Syntax highlighting cache

---

## ✅ Fixed Bugs (History)

### 2026-03-28

**Lua syntax errors (variadic arguments):**

1. `init.lua:34` - `function(...args)` → `function(...)`
2. `core/ui.lua:46` - `function(...args)` → `function(...)`
3. `core/ui.lua:133` - `function(...args)` → `function(...)`
4. `modules/limoon/command_entry.lua:180` - `function(...args)` → `function(...)`
5. `modules/limoon/snippets.lua:635` - `function(...captures)` → `function(...)`
6. `test/helpers.lua:73` - `function(...args)` → `function(...)`
7. `test/helpers.lua:102` - `function(...returns)` → `function(...)`

---

## 🎯 Priorities

### High
1. Fix split view (Bug H)
2. Implement basic LSP
3. Improve error handling in plugins

### Medium
4. Implement TODO menu commands
5. Add tests for the lexer API
6. Improve mouse support

### Low
7. Tab width presets
8. Macro save/load
9. Performance improvements

---

*Last updated: 2026-03-28*
*Generated by: Kimi bug sweep*
