-- Material Dark theme for Li Moon (Mattia Astorino)
local view, colors, styles = view, view.colors, view.styles

colors.bg      = 0x212121 -- #212121
colors.fg      = 0xFFFFEE -- #EEFFFF
colors.comment = 0x7A6E54 -- #546E7A
colors.str     = 0x8DE8C3 -- #C3E88D
colors.kw      = 0xEA92C7 -- #C792EA
colors.func    = 0xFFAA82 -- #82AAFF
colors.num     = 0x6C8CF7 -- #F78C6C
colors.cls     = 0x6BCBFF -- #FFCB6B
colors.builtin = 0xF7DD89 -- #89DDF7
colors.attr    = 0x7871F0 -- #F07178
colors.err     = 0x7053FF -- #FF5370
colors.sel     = 0x2F2F2F -- #2F2F2F
colors.find    = 0x4BB4E8 -- #E8B44B amber
colors.cur     = 0x1A1A1A -- #1A1A1A
colors.lnum    = 0x7A6E54 -- #546E7A

dofile(_HOME .. '/themes/_base.lua')(view, colors, styles)
