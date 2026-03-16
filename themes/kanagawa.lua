-- Kanagawa theme for Textadept (rebelot)
local view, colors, styles = view, view.colors, view.styles

colors.bg      = 0x281F1F -- #1F1F28
colors.fg      = 0xBAD7DC -- #DCD7BA
colors.comment = 0x697172 -- #727169
colors.str     = 0x6CBB98 -- #98BB6C
colors.kw      = 0xB87F95 -- #957FB8
colors.func    = 0xD89C7E -- #7E9CD8
colors.num     = 0x625DFF -- #FF5D62
colors.cls     = 0x9FA87A -- #7AA89F
colors.builtin = 0x997ED2 -- #D27E99
colors.attr    = 0x66A0FF -- #FFA066
colors.err     = 0x4340C3 -- #C34043
colors.sel     = 0x674F2D -- #2D4F67
colors.find    = 0x61A5DC -- #DCA561 autumn yellow
colors.cur     = 0x372A2A -- #2A2A37
colors.lnum    = 0x697172 -- #727169

dofile(_HOME .. '/themes/_base.lua')(view, colors, styles)
