-- Gruvbox Light theme for Textadept (morhetz)
local view, colors, styles = view, view.colors, view.styles

colors.bg      = 0xC7F1FB -- #FBF1C7
colors.fg      = 0x36383C -- #3C3836
colors.comment = 0x748392 -- #928374
colors.str     = 0x0E7479 -- #79740E
colors.kw      = 0x06009D -- #9D0006
colors.func    = 0x587B42 -- #427B58
colors.num     = 0x713F8F -- #8F3F71
colors.cls     = 0x1476B5 -- #B57614
colors.builtin = 0x587B42 -- #427B58
colors.attr    = 0x1980FE -- #FE8019
colors.err     = 0x06009D -- #9D0006
colors.sel     = 0xB2DBEB -- #EBDBB2
colors.find    = 0x0A76E8 -- #E8760A orange on cream
colors.cur     = 0xB2DBEB -- #EBDBB2
colors.lnum    = 0x748392 -- #928374

dofile(_HOME .. '/themes/_base.lua')(view, colors, styles)
