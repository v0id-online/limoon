-- Neon Cyberpunk theme for Li Moon
-- Vibrant neon colors on dark background - inspired by cyberpunk aesthetics

local view, colors, styles = view, view.colors, view.styles

colors.bg      = 0x0A0A12 -- Deep dark blue-black
colors.fg      = 0xE0F0FF -- Bright cyan-white
colors.comment = 0x4A5568 -- Muted blue-grey
colors.str     = 0x39FF14 -- Neon green
colors.kw      = 0xFF10F0 -- Hot pink/magenta
colors.func    = 0x00F0FF -- Cyan
colors.num     = 0xFFAA00 -- Neon orange
colors.cls     = 0xB829F7 -- Purple
colors.builtin = 0x00F5D4 -- Turquoise
colors.attr    = 0xFF3864 -- Neon red-pink
colors.err     = 0xFF073A -- Bright red
colors.sel     = 0x1A1A3E -- Dark purple-blue
colors.find    = 0xFFEA00 -- Neon yellow
colors.cur     = 0x151528 -- Dark blue
colors.lnum    = 0x4A5568 -- Muted blue-grey

dofile(_HOME .. '/themes/_base.lua')(view, colors, styles)

-- Additional neon-specific customizations
view.element_color[view.ELEMENT_CARET] = 0x00F0FF | 0xFF000000 -- Bright cyan caret
