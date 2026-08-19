-- GitHub Dark theme for Li Moon
local view, colors, styles = view, view.colors, view.styles

colors.bg      = 0x17110D -- #0D1117
colors.fg      = 0xD9D1C9 -- #C9D1D9
colors.comment = 0x9E948B -- #8B949E
colors.str     = 0xFFD6A5 -- #A5D6FF
colors.kw      = 0x727BFF -- #FF7B72
colors.func    = 0xFFA8D2 -- #D2A8FF
colors.num     = 0xFFC079 -- #79C0FF
colors.cls     = 0x57A6FF -- #FFA657
colors.builtin = 0xFFC079 -- #79C0FF
colors.attr    = 0x87E77E -- #7EE787
colors.err     = 0x4951F8 -- #F85149
colors.sel     = 0x92642A -- #2A6492
colors.find    = 0x4BB4E8 -- #E8B44B amber
colors.cur     = 0x221B16 -- #161B22
colors.lnum    = 0x81766E -- #6E7681

dofile(_HOME .. '/themes/_base.lua')(view, colors, styles)
