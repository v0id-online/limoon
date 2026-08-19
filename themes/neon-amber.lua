-- Neon Amber theme for Li Moon
-- Amber phosphor meets cyberpunk orange

local view, colors, styles = view, view.colors, view.styles

colors.bg      = 0x0D0802 -- Very dark amber
colors.fg      = 0xFFE4B5 -- Light amber
colors.comment = 0x8B6914 -- Dark goldenrod
colors.str     = 0xFFAA00 -- Amber
colors.kw      = 0xFF6600 -- Neon orange
colors.func    = 0xFFB84D -- Light amber
colors.num     = 0xFFD700 -- Gold
colors.cls     = 0xFF8C00 -- Dark orange
colors.builtin = 0xFFA500 -- Orange
colors.attr    = 0xFF4500 -- Red-orange
colors.err     = 0xFF2222 -- Bright red
colors.sel     = 0x866127 -- #276186
colors.find    = 0xFFFF00 -- Yellow
colors.cur     = 0x261805 -- Very dark amber
colors.lnum    = 0x8B6914 -- Dark goldenrod

dofile(_HOME .. '/themes/_base.lua')(view, colors, styles)

-- Additional neon-specific customizations
view.element_color[view.ELEMENT_CARET] = 0xFFAA00 | 0xFF000000 -- Amber caret
