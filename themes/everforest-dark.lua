-- Everforest Dark theme for Textadept (sainnhe)
local view, colors, styles = view, view.colors, view.styles

colors.bg      = 0x3B352D -- #2D353B
colors.fg      = 0xAAC6D3 -- #D3C6AA
colors.comment = 0x78847A -- #7A8478
colors.str     = 0x80C0A7 -- #A7C080
colors.kw      = 0x807EE6 -- #E67E80
colors.func    = 0xB3BB7F -- #7FBBB3
colors.num     = 0xB699D6 -- #D699B6
colors.cls     = 0x7FBCDB -- #DBBC7F
colors.builtin = 0x92C083 -- #83C092
colors.attr    = 0x807EE6 -- #E67E80
colors.err     = 0x807EE6 -- #E67E80
colors.sel     = 0x41483D -- #3D4841
colors.find    = 0x4BB8E8 -- #E8B84B amber
colors.cur     = 0x36413C -- #3C4136
colors.lnum    = 0x78847A -- #7A8478

dofile(_HOME .. '/themes/_base.lua')(view, colors, styles)
