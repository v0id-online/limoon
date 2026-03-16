-- Base style applicator for community themes.
-- Called at the end of each theme file: require('themes._base')(view, colors, styles)
-- Colors expected in the `colors` table:
--   bg, fg, comment, str, kw, func, num, cls, builtin, attr, err, sel, find, cur, lnum, sep

return function(view, colors, styles)
  if not font then font = WIN32 and 'Consolas' or OSX and 'Monaco' or 'Monospace' end
  if not size then size = not OSX and 10 or 12 end
  colors.sep = colors.sep or colors.sel

  -- Predefined styles
  styles[view.STYLE_DEFAULT]        = {font=font, size=size, fore=colors.fg, back=colors.bg}
  styles[view.STYLE_LINENUMBER]     = {fore=colors.lnum, back=colors.bg}
  styles[view.STYLE_BRACELIGHT]     = {fore=colors.func, bold=true}
  styles[view.STYLE_BRACEBAD]       = {fore=colors.err}
  styles[view.STYLE_INDENTGUIDE]    = {fore=colors.sep}
  styles[view.STYLE_CALLTIP]        = {fore=colors.fg, back=colors.sel}
  styles[view.STYLE_FOLDDISPLAYTEXT]= {fore=colors.comment, back=colors.sel}

  -- Syntax styles
  styles[lexer.ANNOTATION]        = {fore=colors.attr}
  styles[lexer.ATTRIBUTE]         = {fore=colors.attr}
  styles[lexer.BOLD]              = {bold=true}
  styles[lexer.CLASS]             = {fore=colors.cls}
  styles[lexer.CODE]              = {fore=colors.comment, eol_filled=true}
  styles[lexer.COMMENT]           = {fore=colors.comment}
  styles[lexer.CONSTANT_BUILTIN]  = {fore=colors.builtin}
  styles[lexer.EMBEDDED]          = {fore=colors.builtin}
  styles[lexer.ERROR]             = {fore=colors.err}
  styles[lexer.FUNCTION_BUILTIN]  = {fore=colors.builtin}
  styles[lexer.HEADING]           = {fore=colors.kw, bold=true}
  styles[lexer.ITALIC]            = {italic=true}
  styles[lexer.KEYWORD]           = {fore=colors.kw}
  styles[lexer.LABEL]             = {fore=colors.attr}
  styles[lexer.LINK]              = {underline=true}
  styles[lexer.LIST]              = {fore=colors.num}
  styles[lexer.NUMBER]            = {fore=colors.num}
  styles[lexer.PREPROCESSOR]      = {fore=colors.kw}
  styles[lexer.REFERENCE]         = {underline=true}
  styles[lexer.REGEX]             = {fore=colors.str}
  styles[lexer.STRING]            = {fore=colors.str}
  styles[lexer.TAG]               = {fore=colors.attr}
  styles[lexer.TYPE]              = {fore=colors.cls}
  styles[lexer.UNDERLINE]         = {underline=true}
  styles[lexer.VARIABLE_BUILTIN]  = {fore=colors.builtin}

  -- Language-specific
  styles.property            = styles[lexer.ATTRIBUTE]
  styles.addition            = {fore=colors.str}
  styles.deletion            = {fore=colors.err}
  styles.change              = {fore=colors.num}
  styles.tag_unknown         = styles.tag .. {italic=true}
  styles.attribute_unknown   = styles.attribute .. {italic=true}
  styles.command             = styles[lexer.KEYWORD]
  styles.command_section     = styles[lexer.HEADING]
  styles.environment         = styles[lexer.TYPE]
  styles.environment_math    = styles[lexer.NUMBER]
  styles.csi = {visible=false}
  local csi_c = {
    black=colors.bg, red=colors.err, green=colors.str, yellow=colors.num,
    blue=colors.func, magenta=colors.kw, cyan=colors.builtin, white=colors.fg
  }
  for k,v in pairs(csi_c) do styles['csi_'..k]={fore=v} end
  for k,v in pairs(csi_c) do styles['csi_'..k..'_bright']={fore=v,bold=true} end
  styles.keyword_soft = {}
  styles.error_indent = {back=colors.err}

  -- Element colors
  -- Alpha must be 0xFF for IsValid() to return true; otherwise Scintilla
  -- treats the color as "not set" and shows no selection highlight.
  view.element_color[view.ELEMENT_SELECTION_TEXT]                    = colors.fg  | 0xFF000000
  view.element_color[view.ELEMENT_SELECTION_BACK]                    = colors.find | 0xFF000000
  view.element_color[view.ELEMENT_SELECTION_ADDITIONAL_BACK]         = colors.find | 0xFF000000
  view.element_color[view.ELEMENT_SELECTION_SECONDARY_BACK]          = colors.find | 0xFF000000
  view.element_color[view.ELEMENT_SELECTION_INACTIVE_BACK]           = colors.find | 0xFF000000
  view.element_color[view.ELEMENT_SELECTION_INACTIVE_ADDITIONAL_BACK]= colors.find | 0xFF000000
  view.element_color[view.ELEMENT_CARET] = colors.fg
  if view ~= ui.command_entry then
    view.element_color[view.ELEMENT_CARET_LINE_BACK] = colors.cur | 0x80000000
  end
  view.caret_line_layer = view.LAYER_UNDER_TEXT

  -- Fold margin
  view:set_fold_margin_color(true, colors.bg)
  view:set_fold_margin_hi_color(true, colors.bg)

  -- Markers
  view.marker_back[textadept.bookmarks.MARK_BOOKMARK] = colors.func
  view.marker_back[textadept.run.MARK_WARNING]        = colors.num
  view.marker_back[textadept.run.MARK_ERROR]          = colors.err
  view.marker_fore[view.MARKNUM_HISTORY_MODIFIED]     = colors.num
  view.marker_back[view.MARKNUM_HISTORY_MODIFIED]     = colors.num
  view.marker_fore[view.MARKNUM_HISTORY_SAVED]        = colors.str
  view.marker_back[view.MARKNUM_HISTORY_SAVED]        = colors.str
  view.marker_fore[view.MARKNUM_HISTORY_REVERTED_TO_MODIFIED] = colors.num
  view.marker_back[view.MARKNUM_HISTORY_REVERTED_TO_MODIFIED] = colors.num
  view.marker_fore[view.MARKNUM_HISTORY_REVERTED_TO_ORIGIN]   = colors.num
  view.marker_back[view.MARKNUM_HISTORY_REVERTED_TO_ORIGIN]   = colors.num
  for i = view.MARKNUM_FOLDEREND, view.MARKNUM_FOLDEROPEN do
    view.marker_fore[i] = colors.bg
    view.marker_back[i] = colors.comment
    view.marker_back_selected[i] = colors.fg
  end

  -- Indicators
  view.indic_style[ui.find.INDIC_FIND]         = view.INDIC_ROUNDBOX
  view.indic_fore[ui.find.INDIC_FIND]          = colors.find
  view.indic_alpha[ui.find.INDIC_FIND]         = 100
  view.indic_outline_alpha[ui.find.INDIC_FIND] = 200
  view.indic_fore[textadept.editing.INDIC_HIGHLIGHT]    = colors.func
  view.indic_alpha[textadept.editing.INDIC_HIGHLIGHT]   = 0x80
  view.indic_fore[textadept.snippets.INDIC_PLACEHOLDER] = colors.fg
  view.indic_fore[textadept.run.INDIC_WARNING]          = colors.num
  view.indic_fore[textadept.run.INDIC_ERROR]            = colors.err

  -- Call tips & long lines
  view.call_tip_fore_hlt = colors.func
  view.edge_color = colors.sel
  ui.find.entry_font = font .. ' ' .. size
end
