-- Monokai theme for Li Moon (Wimer Hazenberg)
local view, colors, styles = view, view.colors, view.styles

-- Colors (0xBBGGRR)
colors.bg      = 0x222827 -- #272822
colors.fg      = 0xF2F8F8 -- #F8F8F2
colors.comment = 0x5E7175 -- #75715E
colors.str     = 0x74DBE6 -- #E6DB74
colors.kw      = 0x7226F9 -- #F92672
colors.func    = 0x2EE2A6 -- #A6E22E
colors.num     = 0xFF81AE -- #AE81FF
colors.cls     = 0xE8D966 -- #66D9E8
colors.builtin = 0xE8D966 -- #66D9E8
colors.attr    = 0x2EE2A6 -- #A6E22E
colors.err     = 0x2F00CC -- #CC002F
colors.sel     = 0x3E4849 -- #49483E
colors.find    = 0x1F97FD -- #FD971F monokai orange
colors.cur     = 0x323D3E -- #3E3D32
colors.lnum    = 0x5E7175 -- #75715E

dofile(_HOME .. '/themes/_base.lua')(view, colors, styles)
