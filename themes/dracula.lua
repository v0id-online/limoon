-- Dracula theme for Li Moon (dracula.github.io)
local view, colors, styles = view, view.colors, view.styles

colors.bg      = 0x362A28 -- #282A36
colors.fg      = 0xF2F8F8 -- #F8F8F2
colors.comment = 0xA47262 -- #6272A4
colors.str     = 0x8CFAF1 -- #F1FA8C
colors.kw      = 0xC679FF -- #FF79C6
colors.func    = 0x7BFA50 -- #50FA7B
colors.num     = 0xF993BD -- #BD93F9
colors.cls     = 0xFDE98B -- #8BE9FD
colors.builtin = 0x6CB8FF -- #FFB86C
colors.attr    = 0x7BFA50 -- #50FA7B
colors.err     = 0x2F72FF -- #FF722F
colors.sel     = 0xB46F64 -- #646FB4
colors.find    = 0x4BB8E8 -- #E8B84B amber
colors.cur     = 0x5A4744 -- #44475A
colors.lnum    = 0xA47262 -- #6272A4

dofile(_HOME .. '/themes/_base.lua')(view, colors, styles)
