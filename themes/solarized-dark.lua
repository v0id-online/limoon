-- Solarized Dark theme for Li Moon (Ethan Schoonover)
local view, colors, styles = view, view.colors, view.styles

colors.bg      = 0x362B00 -- #002B36
colors.fg      = 0x969483 -- #839496
colors.comment = 0x756E58 -- #586E75
colors.str     = 0x98A12A -- #2AA198
colors.kw      = 0x009985 -- #859900
colors.func    = 0xD28B26 -- #268BD2
colors.num     = 0x8236D3 -- #D33682
colors.cls     = 0x0089B5 -- #B58900
colors.builtin = 0x164BCB -- #CB4B16
colors.attr    = 0x837B65 -- #657B83
colors.err     = 0x2F32DC -- #DC322F
colors.sel     = 0x8E7A29 -- #297A8E
colors.find    = 0x20A8E8 -- #E8A820 amber
colors.cur     = 0x423607 -- #073642
colors.lnum    = 0x756E58 -- #586E75

dofile(_HOME .. '/themes/_base.lua')(view, colors, styles)
