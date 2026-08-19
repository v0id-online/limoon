-- Catppuccin Mocha theme for Li Moon (catppuccin)
local view, colors, styles = view, view.colors, view.styles

colors.bg      = 0x2E1E1E -- #1E1E2E
colors.fg      = 0xF4D6CD -- #CDD6F4
colors.comment = 0x86706C -- #6C7086
colors.str     = 0xA1E3A6 -- #A6E3A1
colors.kw      = 0xF7A6CB -- #CBA6F7
colors.func    = 0xFAB489 -- #89B4FA
colors.num     = 0x87B3FA -- #FAB387
colors.cls     = 0xAFE2F9 -- #F9E2AF
colors.builtin = 0xEBDC89 -- #89DCEB
colors.attr    = 0xA88BF3 -- #F38BA8
colors.err     = 0xA88BF3 -- #F38BA8
colors.sel     = 0xB1625D -- #5D62B1
colors.find    = 0x6AC4E8 -- #E8C46A amber
colors.cur     = 0x251818 -- #181825
colors.lnum    = 0x705B58 -- #585B70

dofile(_HOME .. '/themes/_base.lua')(view, colors, styles)
