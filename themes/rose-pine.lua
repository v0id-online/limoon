-- Rosé Pine theme for Textadept (rose-pine)
local view, colors, styles = view, view.colors, view.styles

colors.bg      = 0x241719 -- #191724
colors.fg      = 0xF4DEE0 -- #E0DEF4
colors.comment = 0x866A6E -- #6E6A86
colors.str     = 0x77C1F6 -- #F6C177
colors.kw      = 0xE7A7C4 -- #C4A7E7
colors.func    = 0xD8CF9C -- #9CCFD8
colors.num     = 0x926FEB -- #EB6F92
colors.cls     = 0xBABCEB -- #EBBCBA
colors.builtin = 0x8F7431 -- #31748F
colors.attr    = 0x8F7431 -- #31748F
colors.err     = 0x926FEB -- #EB6F92
colors.sel     = 0x3A2326 -- #26233A
colors.find    = 0x349DEA -- #EA9D34 amber-orange
colors.cur     = 0x2E1D1F -- #1F1D2E
colors.lnum    = 0x866A6E -- #6E6A86

dofile(_HOME .. '/themes/_base.lua')(view, colors, styles)
