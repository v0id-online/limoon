-- Tomorrow Night theme for Li Moon (Chris Kempson)
local view, colors, styles = view, view.colors, view.styles

colors.bg      = 0x211F1D -- #1D1F21
colors.fg      = 0xC6C8C5 -- #C5C8C6
colors.comment = 0x969896 -- #969896
colors.str     = 0x68BDB5 -- #B5BD68
colors.kw      = 0xBB94B2 -- #B294BB
colors.func    = 0xBEA281 -- #81A2BE
colors.num     = 0x5F93DE -- #DE935F
colors.cls     = 0x74C6F0 -- #F0C674
colors.builtin = 0xBEA281 -- #81A2BE
colors.attr    = 0x6666CC -- #CC6666
colors.err     = 0x6666CC -- #CC6666
colors.sel     = 0x413B37 -- #373B41
colors.find    = 0x4BB4E8 -- #E8B44B amber
colors.cur     = 0x2E2A28 -- #282A2E
colors.lnum    = 0x969896 -- #969896

dofile(_HOME .. '/themes/_base.lua')(view, colors, styles)
