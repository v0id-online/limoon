-- Solarized Light theme for Textadept (Ethan Schoonover)
local view, colors, styles = view, view.colors, view.styles

colors.bg      = 0xE3F6FD -- #FDF6E3
colors.fg      = 0x837B65 -- #657B83
colors.comment = 0xA1A193 -- #93A1A1
colors.str     = 0x98A12A -- #2AA198
colors.kw      = 0x009985 -- #859900
colors.func    = 0xD28B26 -- #268BD2
colors.num     = 0x8236D3 -- #D33682
colors.cls     = 0x0089B5 -- #B58900
colors.builtin = 0x164BCB -- #CB4B16
colors.attr    = 0x756E58 -- #586E75
colors.err     = 0x2F32DC -- #DC322F
colors.sel     = 0xD5E8EE -- #EEE8D5
colors.find    = 0x008AE8 -- #E88A00 orange on cream
colors.cur     = 0xD5E8EE -- #EEE8D5
colors.lnum    = 0xA1A193 -- #93A1A1

dofile(_HOME .. '/themes/_base.lua')(view, colors, styles)
