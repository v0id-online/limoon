-- Vaporwave theme for Li Moon
-- Aesthetic purples, pinks, and cyans - sunset vibes

local view, colors, styles = view, view.colors, view.styles

colors.bg      = 0x1A0A2E -- Deep purple
colors.fg      = 0xFFB8D0 -- Light pink
colors.comment = 0x8B5A8C -- Muted mauve
colors.str     = 0x00FFFF -- Cyan
colors.kw      = 0xFF00FF -- Magenta
colors.func    = 0xFF69B4 -- Hot pink
colors.num     = 0xFFD700 -- Gold
colors.cls     = 0xDA70D6 -- Orchid
colors.builtin = 0x40E0D0 -- Turquoise
colors.attr    = 0xFF1493 -- Deep pink
colors.err     = 0xFF3333 -- Red
colors.sel     = 0x7A32AE -- #AE327A
colors.find    = 0xFFA500 -- Orange
colors.cur     = 0x2E1A47 -- Dark purple
colors.lnum    = 0x8B5A8C -- Muted mauve

dofile(_HOME .. '/themes/_base.lua')(view, colors, styles)

-- Additional vaporwave-specific customizations
view.element_color[view.ELEMENT_CARET] = 0xFF69B4 | 0xFF000000 -- Pink caret
