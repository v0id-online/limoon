-- Matrix theme for Li Moon
-- The green digital rain aesthetic

local view, colors, styles = view, view.colors, view.styles

colors.bg      = 0x000000 -- Pure black
colors.fg      = 0x00FF41 -- Matrix green
colors.comment = 0x008F11 -- Dark matrix green
colors.str     = 0x00FF00 -- Bright green
colors.kw      = 0x00CC00 -- Medium green
colors.func    = 0x66FF66 -- Light green
colors.num     = 0x99FF99 -- Pale green
colors.cls     = 0x00FF88 -- Green-cyan
colors.builtin = 0x44FF44 -- Light-medium green
colors.attr    = 0x00FF41 -- Matrix green
colors.err     = 0xFF0040 -- Red pill
colors.sel     = 0x003B00 -- Very dark green
colors.find    = 0xCCFF00 -- Yellow-green
colors.cur     = 0x001500 -- Almost black green
colors.lnum    = 0x008F11 -- Dark matrix green

dofile(_HOME .. '/themes/_base.lua')(view, colors, styles)

-- Matrix-specific customizations
view.element_color[view.ELEMENT_CARET] = 0x00FF41 | 0xFF000000 -- Matrix green caret
