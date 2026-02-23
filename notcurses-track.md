# Notcurses Implementation Tracking

Data de início: 2026-02-22

Este arquivo rastreia as alterações e progresso da implementação do Textadept usando Notcurses.

## Objetivo

Substituir a interface baseada em ncurses (atualmente via CDK?) pela biblioteca Notcurses para melhor suporte a gráficos, cores, e funcionalidades modernas em terminais.

## Tarefas

- [x] Analisar a estrutura atual do backend curses (`src/textadept_curses.c`)
- [x] Criar um novo backend `src/n_textadept.c`
- [ ] Adaptar a construção (Makefile, CMake) para incluir notcurses
- [ ] Implementar as funções de plataforma necessárias (set_title, focus_view, etc.)
- [ ] Testar a integração
- [ ] Documentar

## Mapeamento de Funções da Plataforma

Com base em `textadept_platform.h`, a implementação Notcurses deve fornecer as seguintes funções:

| Função | Status |
|--------|--------|
| `get_platform` | pendente |
| `get_charset` | pendente |
| `new_window` | pendente |
| `set_title` | stub |
| `is_maximized` | pendente |
| `get_size` | pendente |
| `set_size` | pendente |
| `new_scintilla` | pendente |
| `focus_view` | stub |
| `SS` | pendente |
| `split_view` | pendente |
| `unsplit_view` | pendente |
| `delete_scintilla` | pendente |
| `get_top_pane` | pendente |
| `get_pane_info` | pendente |
| `get_parent_pane_info` | pendente |
| `get_pane_info_from_view` | pendente |
| `set_pane_split_pos` | pendente |
| `show_tabs` | pendente |
| `add_tab` | pendente |
| `set_tab` | pendente |
| `set_tab_label` | pendente |
| `move_tab` | pendente |
| `remove_tab` | pendente |
| `get_find_text` | pendente |
| `get_repl_text` | pendente |
| `set_find_text` | pendente |
| `set_repl_text` | pendente |
| `add_to_find_history` | pendente |
| `add_to_repl_history` | pendente |
| `set_entry_font` | pendente |
| `is_checked` | pendente |
| `toggle` | pendente |
| `set_find_label` | pendente |
| `set_repl_label` | pendente |
| `set_button_label` | pendente |
| `set_option_label` | pendente |
| `focus_find` | pendente |
| `focus_command_entry` | pendente |
| `is_command_entry_active` | pendente |
| `set_command_entry_label` | pendente |
| `get_command_entry_height` | pendente |
| `set_command_entry_height` | pendente |
| `is_statusbar_visible` | pendente |
| `set_statusbar_visible` | pendente |
| `get_statusbar_text` | pendente |
| `set_statusbar_text` | pendente |
| `read_menu` | pendente |
| `popup_menu` | pendente |
| `set_menubar` | pendente |
| `get_clipboard_text` | stub (retorna NULL) |
| `add_timeout` | pendente |
| `update_ui` | pendente |
| `is_hidpi` | pendente |
| `is_dark_mode` | pendente |
| `message_dialog` | pendente |
| `input_dialog` | pendente |
| `open_dialog` | pendente |
| `save_dialog` | pendente |
| `progress_dialog` | pendente |
| `list_dialog` | pendente |
| `spawn` | pendente |
| `process_size` | pendente |
| `is_process_running` | pendente |
| `wait_process` | pendente |
| `read_process_output` | pendente |
| `write_process_input` | pendente |
| `close_process_input` | pendente |
| `kill_process` | pendente |
| `get_process_exit_status` | pendente |
| `cleanup_process` | pendente |
| `suspend` | pendente |
| `quit` | pendente |

## Registro de alterações

### 2026-02-22
- Criado este arquivo de rastreamento.

### 2026-02-22 (continuação)
- Criado arquivo boilerplate `src/n_textadept.c` com includes de Notcurses e stubs de funções.
- Ainda é necessário analisar o Makefile e adicionar alvo de compilação (aguardando adição do arquivo Makefile ao chat).

### 2026-02-22 (mais tarde)
- Analisado cabeçalho `textadept_platform.h` e mapeadas todas as funções a serem implementadas.
- Adicionada tabela de mapeamento no arquivo de tracking.
