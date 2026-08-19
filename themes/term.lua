-- Terminal theme for Li Moon
-- Classic 16-color terminal palette

local view, colors, styles = view, view.colors, view.styles

colors.bg      = 0x000000 -- Black
colors.fg      = 0xC0C0C0 -- White (light grey)
colors.comment = 0x808080 -- Light black (dark grey)
colors.str     = 0x00FF00 -- Light green
colors.kw      = 0x0000FF -- Light blue
colors.func    = 0x00FFFF -- Light cyan
colors.num     = 0xFF00FF -- Light magenta
colors.cls     = 0xFFFF00 -- Light yellow
colors.builtin = 0x008000 -- Green
colors.attr    = 0x800000 -- Red
colors.err     = 0xFF0000 -- Light red
colors.sel     = 0x5C5C5C -- #5C5C5C
colors.find    = 0x00FF00 -- Light green
colors.cur     = 0x202020 -- Dark grey
colors.lnum    = 0x808080 -- Light black

dofile(_HOME .. '/themes/_base.lua')(view, colors, styles)
