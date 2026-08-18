# Li Moon - Feature Roadmap

> Prioritized features for implementation
> Date: 2026-03-28

---

## 🔥 PRIORITY 1 - Implement Immediately

### 1. Block Selection

**Description:** Select text in block/column format (rectangular selection)
**Shortcuts:**
- `Alt+Shift+Arrows` - Extend rectangular selection
- `Ctrl+Shift+B` - Start block selection in normal mode
- With mouse: `Alt+Click+Drag`

**Use:**
- Edit multiple lines simultaneously
- Select columns of data
- Comment/uncomment blocks of code

**Implementation:** Use Scintilla's rectangular selection API (`SCI_SETSELECTIONMODE`, `SCI_SETRECTANGULARSELECTIONMODE`)

---

### 2. Auto-pairs (Smart Brackets)

**Description:** Auto-complete character pairs
**Characters:** `()`, `{}`, `[]`, `""`, `''`, `` `` ``

**Behavior:**
- Type `(` → inserts `()` and places cursor inside
- Type `)` when `)` already exists → just moves cursor out
- Selection + `(` → wraps selection in parentheses
- Backspace on empty `()` → removes both

**Per-language configuration:**
```lua
autopairs.lua = { '(', ')', '{', '}', '[', ']', '"', "'" }
autopairs.rust = { '(', ')', '{', '}', '[', ']', '"', "'", '<', '>' }
```

---

### 3. Fuzzy File Finder (Ctrl+Shift+F)

**Description:** Quick file search across the project
**Command:** `fzf()` or shortcut `Ctrl+Shift+F`

**Features:**
- Fuzzy search (no need to type exactly)
- Preview of the selected file
- Ignores files in `.gitignore`
- Shows icon per file type

**Interface:**
```
┌─────────────────────────────────────────┐
│ > config                                │
├─────────────────────────────────────────┤
│   src/config.lua              [lua]     │
│   core/config.lua             [lua]     │
│   test/config_test.lua        [lua]     │
│   docs/config.md              [md]      │
└─────────────────────────────────────────┘
```

---

### 4. Highlight Word Under Cursor

**Description:** Highlight all occurrences of the current word
**Shortcut:** Automatic when cursor is placed

**Behavior:**
- Cursor on "function" → all "function" occurrences in the file are highlighted
- Different color from selection (e.g. soft yellow)
- Ignores case partially (configurable)

**API:** Use Scintilla indicators (`SCI_INDICATORFILLRANGE`)

---

## ⚡ PRIORITY 2 - Implement Next

### 5. Integrated Terminal (Ctrl+`)

**Description:** Terminal embedded in the editor
**Commands:**
```lua
term()              -- Opens terminal at the bottom
term("horizontal")  -- Horizontal split
term("vertical")    -- Vertical split
term.close()        -- Closes terminal
```

**Features:**
- Multiple terminal tabs
- Preserves history across sessions
- Integration with project environment variables

---

### 6. Indent Guides

**Description:** Vertical lines showing indentation levels
**Visual:**
```lua
function example()
│   if true then
│   │   print("hello")
│   │   -- guide line here
│   end
end
```

**Configuration:**
```lua
view.indent_guide_color = 0x555555
view.indent_guide_style = "dotted"  -- or "solid", "dashed"
```

---

### 7. Relative Line Numbers

**Description:** Line numbers relative to the current position
```
5  -- code
4  -- code
3  -- code
2  -- code
1  -- code
0  cursor here  <-- current line
1  -- code
2  -- code
3  -- code
```

**Helps with:** Movements like `5j`, `10k` in Vim mode

---

### 8. Auto-save

**Description:** Automatically save files
**Command:** `autosave(seconds)`

**Options:**
```lua
autosave(3)           -- Save after 3 seconds of inactivity
autosave("focus_lost") -- Save when focus is lost
autosave("change")     -- Save on every change (debounced)
```

---

## 📋 PRIORITY 3 - Advanced Features

### 9. Git Gutter

**Description:** Git change indicators in the margin
**Symbols:**
- `▐` modified line (yellow)
- `▐` added line (green)
- `▐` removed line (red)

**Position:** Left margin (or overlay on line numbers)

---

### 10. Persistent Workspace/Session

**Description:** Save and restore work sessions
**Commands:**
```lua
workspace.save("project-x")  -- Save current workspace
workspace.load("project-x")  -- Restore workspace
workspace.list()             -- List saved workspaces
```

**Persists:**
- Open files
- Cursor positions
- Splits and their configuration
- Command history

---

### 11. Persistent Bookmarks

**Description:** Mark positions in code
**Shortcuts:**
```
Ctrl+Shift+M      -- Toggle bookmark at current position
Ctrl+,            -- Next bookmark
Ctrl+.            -- Previous bookmark
```

**Visual:** Icon in the margin (e.g. 🔖)

---

### 12. Rainbow Parentheses

**Description:** Different colors per nesting level
```lua
function()           -- ( blue
  return function()  -- ( green
    if true then     -- ( yellow
      print("x")     
    end              -- ) yellow
  end                -- ) green
end                  -- ) blue
```

---

## 🛠️ Implementation Notes

### Block Selection (PRIORITY 1)
**Files involved:**
- `src/n_limoon.c` - Add Alt+Shift+mouse handling
- `core/view.lua` - Expose rectangular selection functions
- `init.lua` - Configure keybindings

**Scintilla reference:**
```c
SCI_SETRECTANGULARSELECTIONANCHOR(pos)
SCI_SETRECTANGULARSELECTIONCARET(pos)
```

### Auto-pairs
**File:** `modules/autopairs.lua` (new)
**Hook:** `events.KEY` to intercept typed characters

### Fuzzy Finder
**File:** `modules/fuzzy_finder.lua` (new)
**Dependency:** `find` or `fd` (external tool)
**UI:** Reuse `ui.dialogs.list` or build a custom one

### Word Highlight
**File:** `modules/word_highlight.lua` (new)
**Event:** `events.UPDATE_UI`
**Delay:** 300ms debounce to avoid lag

---

## ✅ Implementation Checklist

- [ ] Block Selection
- [ ] Auto-pairs (Smart Brackets)
- [ ] Fuzzy File Finder
- [ ] Highlight Word Under Cursor
- [ ] Integrated Terminal
- [ ] Indent Guides
- [ ] Relative Line Numbers
- [ ] Auto-save
- [ ] Git Gutter
- [ ] Persistent Workspace
- [ ] Persistent Bookmarks
- [ ] Rainbow Parentheses

---

## 🎯 Next Steps

1. **Choose what to implement first**
2. **Create a branch** for the feature
3. **Test** across different scenarios
4. **Document** in the help (Ctrl+H)
5. **Add** to the CHANGELOG

---

*Updated: 2026-03-28*
