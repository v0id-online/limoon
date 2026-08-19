-- Argonaut theme for Li Moon
-- Based on the classic terminal theme with vibrant colors

local view, colors, styles = view, view.colors, view.styles

colors.bg      = 0x121212 -- #121212
colors.fg      = 0xE5E5E5 -- #E5E5E5
colors.comment = 0x666666 -- #666666
colors.str     = 0x55FF55 -- #55FF55 (bright green)
colors.kw      = 0x5555FF -- #5555FF (bright blue)
colors.func    = 0xFF9B3B -- #FF9B3B (orange)
colors.num     = 0xFFFF55 -- #FFFF55 (yellow)
colors.cls     = 0x55FFFF -- #55FFFF (cyan)
colors.builtin = 0xCC0066 -- #CC0066 (magenta)
colors.attr    = 0xFF9B55 -- #FF9B55 (light orange)
colors.err     = 0xFF5555 -- #FF5555 (red)
colors.sel     = 0x616161 -- #616161
colors.find    = 0xFFCC00 -- #00CCFF (bright cyan)
colors.cur     = 0x1A1A1A -- #1A1A1A
colors.lnum    = 0x666666 -- #666666

dofile(_HOME .. '/themes/_base.lua')(view, colors, styles)
