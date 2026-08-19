-- Nord theme for Li Moon (Arctic Ice Studio)
local view, colors, styles = view, view.colors, view.styles

colors.bg      = 0x40342E -- #2E3440
colors.fg      = 0xE9DED8 -- #D8DEE9
colors.comment = 0x6A564C -- #4C566A
colors.str     = 0x8CBEA3 -- #A3BE8C
colors.kw      = 0xC1A181 -- #81A1C1
colors.func    = 0xD0C088 -- #88C0D0
colors.num     = 0xAD8EB4 -- #B48EAD
colors.cls     = 0xBBBC8F -- #8FBCBB
colors.builtin = 0xC1A181 -- #81A1C1
colors.attr    = 0x6A61BF -- #BF616A
colors.err     = 0x6A61BF -- #BF616A
colors.sel     = 0xB47D64 -- #647DB4
colors.find    = 0x8BCBEB -- #EBCB8B aurora yellow
colors.cur     = 0x52423B -- #3B4252
colors.lnum    = 0x6A564C -- #4C566A

dofile(_HOME .. '/themes/_base.lua')(view, colors, styles)
