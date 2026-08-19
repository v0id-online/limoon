-- Neon Blue theme for Li Moon
-- Electric blue and cyan neon aesthetic

local view, colors, styles = view, view.colors, view.styles

colors.bg      = 0x050A14 -- Deep blue-black
colors.fg      = 0xE0F7FF -- Ice white
colors.comment = 0x4A6572 -- Steel blue
colors.str     = 0x00D9FF -- Electric blue
colors.kw      = 0x0080FF -- Bright blue
colors.func    = 0x00FFFF -- Cyan
colors.num     = 0x7DF9FF -- Electric cyan
colors.cls     = 0x0099FF -- Azure
colors.builtin = 0x00CCFF -- Sky blue
colors.attr    = 0x00FFCC -- Aqua
colors.err     = 0xFF3366 -- Hot pink
colors.sel     = 0x275286 -- #865227
colors.find    = 0xFFFF00 -- Electric yellow
colors.cur     = 0x0A1A2E -- Very dark blue
colors.lnum    = 0x4A6572 -- Steel blue

dofile(_HOME .. '/themes/_base.lua')(view, colors, styles)

-- Additional neon-specific customizations
view.element_color[view.ELEMENT_CARET] = 0x00FFFF | 0xFF000000 -- Cyan caret
