-- Ayu Dark theme for Li Moon (dempfi)
local view, colors, styles = view, view.colors, view.styles

colors.bg      = 0x140E0B -- #0B0E14
colors.fg      = 0xB6BDBF -- #BFBDB6
colors.comment = 0x736A62 -- #626A73
colors.str     = 0x4CD9AA -- #AAD94C
colors.kw      = 0x408FFF -- #FF8F40
colors.func    = 0x54B4FF -- #FFB454
colors.num     = 0x73B6E6 -- #E6B673
colors.cls     = 0xE6BA39 -- #39BAE6
colors.builtin = 0x54B4FF -- #FFB454
colors.attr    = 0x6896F2 -- #F29668
colors.err     = 0x3333FF -- #FF3333
colors.sel     = 0x503815 -- #153850
colors.find    = 0x50B4E6 -- #E6B450 amber
colors.cur     = 0x261B0D -- #0D1B26
colors.lnum    = 0x736A62 -- #626A73

dofile(_HOME .. '/themes/_base.lua')(view, colors, styles)
