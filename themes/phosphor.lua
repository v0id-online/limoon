-- Phosphor theme for Li Moon
-- Retro green phosphor CRT monitor style

local view, colors, styles = view, view.colors, view.styles

colors.bg      = 0x050805 -- Very dark green-black
colors.fg      = 0x66FF66 -- Bright phosphor green
colors.comment = 0x336633 -- Dim green
colors.str     = 0x88FF88 -- Light phosphor green
colors.kw      = 0x44FF44 -- Medium phosphor green
colors.func    = 0x77FF77 -- Phosphor green
colors.num     = 0x99FF99 -- Light phosphor green
colors.cls     = 0xAAFFAA -- Very light green
colors.builtin = 0x55FF55 -- Medium-bright green
colors.attr    = 0x66FF66 -- Bright phosphor green
colors.err     = 0xFF6666 -- Red (for errors only)
colors.sel     = 0x0F3A0F -- Dark green
colors.find    = 0x00FF00 -- Pure green
colors.cur     = 0x0A1A0A -- Very dark green
colors.lnum    = 0x336633 -- Dim green

dofile(_HOME .. '/themes/_base.lua')(view, colors, styles)
