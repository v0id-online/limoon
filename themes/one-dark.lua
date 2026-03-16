-- One Dark theme for Textadept (Atom)
local view, colors, styles = view, view.colors, view.styles

colors.bg      = 0x342C28 -- #282C34
colors.fg      = 0xBFB2AB -- #ABB2BF
colors.comment = 0x70635C -- #5C6370
colors.str     = 0x79C398 -- #98C379
colors.kw      = 0xDD78C6 -- #C678DD
colors.func    = 0xEFAF61 -- #61AFEF
colors.num     = 0x669AD1 -- #D19A66
colors.cls     = 0x7BC0E5 -- #E5C07B
colors.builtin = 0xC2B656 -- #56B6C2
colors.attr    = 0x756CE0 -- #E06C75
colors.err     = 0x756CE0 -- #E06C75
colors.sel     = 0x51443E -- #3E4451
colors.find    = 0x4BB4E8 -- #E8B44B amber
colors.cur     = 0x3C312C -- #2C313C
colors.lnum    = 0x63524B -- #4B5263

dofile(_HOME .. '/themes/_base.lua')(view, colors, styles)
