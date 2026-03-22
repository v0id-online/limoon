-- Ayu Mirage theme for Li Moon (dempfi)
local view, colors, styles = view, view.colors, view.styles

colors.bg      = 0x332721 -- #212733
colors.fg      = 0xC6CCCB -- #CBCCC6
colors.comment = 0x73675C -- #5C6773
colors.str     = 0x7EE6BA -- #BAE67E
colors.kw      = 0x66ADFF -- #FFAD66
colors.func    = 0x80D5FF -- #FFD580
colors.num     = 0xFFBFDF -- #DFBFFF
colors.cls     = 0xE6CF5C -- #5CCFE6
colors.builtin = 0xFFD073 -- #73D0FF
colors.attr    = 0xFFD073 -- #73D0FF
colors.err     = 0x3333FF -- #FF3333
colors.sel     = 0x624434 -- #344462
colors.find    = 0x50B4E6 -- #E6B450 amber
colors.cur     = 0x3F2A1F -- #1F2A3F
colors.lnum    = 0x73675C -- #5C6773

dofile(_HOME .. '/themes/_base.lua')(view, colors, styles)
