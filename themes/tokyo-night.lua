-- Tokyo Night theme for Li Moon (enkia)
local view, colors, styles = view, view.colors, view.styles

colors.bg      = 0x261B1A -- #1A1B26
colors.fg      = 0xD6B1A9 -- #A9B1D6
colors.comment = 0x895F56 -- #565F89
colors.str     = 0x6ACE9E -- #9ECE6A
colors.kw      = 0xF79ABB -- #BB9AF7
colors.func    = 0xF7A27A -- #7AA2F7
colors.num     = 0x649EFF -- #FF9E64
colors.cls     = 0xDEC32A -- #2AC3DE
colors.builtin = 0xFFCF7D -- #7DCFFF
colors.attr    = 0x8E76F7 -- #F7768E
colors.err     = 0x8E76F7 -- #F7768E
colors.sel     = 0xA8644D -- #4D64A8
colors.find    = 0x68AFE0 -- #E0AF68 gold
colors.cur     = 0x35231F -- #1F2335
colors.lnum    = 0x61423B -- #3B4261

dofile(_HOME .. '/themes/_base.lua')(view, colors, styles)
