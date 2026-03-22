-- GitHub Light theme for Li Moon
local view, colors, styles = view, view.colors, view.styles

colors.bg      = 0xFFFFFF -- #FFFFFF
colors.fg      = 0x2F2924 -- #24292F
colors.comment = 0x81776E -- #6E7781
colors.str     = 0x69300A -- #0A3069
colors.kw      = 0x2E22CF -- #CF222E
colors.func    = 0xDF5082 -- #8250DF
colors.num     = 0xAE5005 -- #0550AE
colors.cls     = 0x003895 -- #953800
colors.builtin = 0xAE5005 -- #0550AE
colors.attr    = 0x296311 -- #116329
colors.err     = 0x2E22CF -- #CF222E
colors.sel     = 0xFFE3B6 -- #B6E3FF
colors.find    = 0x0086FF -- #FF8600 orange on white
colors.cur     = 0xFAF8F6 -- #F6F8FA
colors.lnum    = 0x81776E -- #6E7781

dofile(_HOME .. '/themes/_base.lua')(view, colors, styles)
