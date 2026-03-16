-- Catppuccin Latte theme for Textadept (catppuccin - light variant)
local view, colors, styles = view, view.colors, view.styles

colors.bg      = 0xF5F1EF -- #EFF1F5
colors.fg      = 0x694F4C -- #4C4F69
colors.comment = 0xA18F8C -- #8C8FA1
colors.str     = 0x2BA040 -- #40A02B
colors.kw      = 0xEF3988 -- #8839EF
colors.func    = 0xF5661E -- #1E66F5
colors.num     = 0x0B64FE -- #FE640B
colors.cls     = 0x1D8EDF -- #DF8E1D
colors.builtin = 0xE5A504 -- #04A5E5
colors.attr    = 0x390FD2 -- #D20F39
colors.err     = 0x390FD2 -- #D20F39
colors.sel     = 0xDAD0CC -- #CCD0DA
colors.find    = 0x0AA5E5 -- #E5A50A amber on light
colors.cur     = 0xE8E0DC -- #DCE0E8
colors.lnum    = 0xA18F8C -- #8C8FA1

dofile(_HOME .. '/themes/_base.lua')(view, colors, styles)
