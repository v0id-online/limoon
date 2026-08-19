-- High Contrast theme for Li Moon
-- Maximum contrast for accessibility

local view, colors, styles = view, view.colors, view.styles

colors.bg      = 0x000000 -- Pure black
colors.fg      = 0xFFFFFF -- Pure white
colors.comment = 0x808080 -- Grey
colors.str     = 0x00FF00 -- Bright green
colors.kw      = 0xFFFF00 -- Bright yellow
colors.func    = 0x00FFFF -- Bright cyan
colors.num     = 0xFF00FF -- Bright magenta
colors.cls     = 0xFF8000 -- Orange
colors.builtin = 0x0080FF -- Light blue
colors.attr    = 0xFF8080 -- Light red
colors.err     = 0xFF0000 -- Pure red
colors.sel     = 0x5C5C5C -- #5C5C5C
colors.find    = 0x00FF00 -- Bright green
colors.cur     = 0x202020 -- Very dark grey
colors.lnum    = 0x808080 -- Grey

dofile(_HOME .. '/themes/_base.lua')(view, colors, styles)
