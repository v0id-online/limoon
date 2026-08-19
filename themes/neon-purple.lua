-- Neon Purple theme for Li Moon
-- Purple and pink neon glow aesthetic

local view, colors, styles = view, view.colors, view.styles

colors.bg      = 0x120A1A -- Deep purple-black
colors.fg      = 0xF5E6FF -- Light lavender
colors.comment = 0x6B4C7A -- Muted purple
colors.str     = 0xD946EF -- Neon fuchsia
colors.kw      = 0xA855F7 -- Neon purple
colors.func    = 0xE879F9 -- Light pink
colors.num     = 0xF472B6 -- Hot pink
colors.cls     = 0xC084FC -- Light purple
colors.builtin = 0xF0ABFC -- Pale pink
colors.attr    = 0xFF00FF -- Magenta
colors.err     = 0xFF0066 -- Bright pink-red
colors.sel     = 0x603AA7 -- #A73A60
colors.find    = 0xFFD700 -- Gold
colors.cur     = 0x1E1035 -- Very dark purple
colors.lnum    = 0x6B4C7A -- Muted purple

dofile(_HOME .. '/themes/_base.lua')(view, colors, styles)

-- Additional neon-specific customizations
view.element_color[view.ELEMENT_CARET] = 0xE879F9 | 0xFF000000 -- Pink caret
