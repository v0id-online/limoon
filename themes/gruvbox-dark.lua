-- Gruvbox Dark theme for Textadept (morhetz)
local view, colors, styles = view, view.colors, view.styles

colors.bg      = 0x282828 -- #282828
colors.fg      = 0xB2DBEB -- #EBDBB2
colors.comment = 0x748392 -- #928374
colors.str     = 0x26BBB8 -- #B8BB26
colors.kw      = 0x3449FB -- #FB4934
colors.func    = 0x7CC08E -- #8EC07C
colors.num     = 0x9B86D3 -- #D3869B
colors.cls     = 0x2FBDFA -- #FABD2F
colors.builtin = 0x98A583 -- #83A598
colors.attr    = 0x1980FE -- #FE8019
colors.err     = 0x3449FB -- #FB4934
colors.sel     = 0x36383C -- #3C3836
colors.find    = 0x2199D7 -- #D79921 gruvbox yellow
colors.cur     = 0x36383C -- #3C3836
colors.lnum    = 0x748392 -- #928374

dofile(_HOME .. '/themes/_base.lua')(view, colors, styles)
