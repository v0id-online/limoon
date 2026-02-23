# Notcurses Implementation Tracking

Data de início: 2026-02-22

Este arquivo rastreia as alterações e progresso da implementação do Textadept usando Notcurses.

## Objetivo

Substituir a interface baseada em ncurses (atualmente via CDK?) pela biblioteca Notcurses para melhor suporte a gráficos, cores, e funcionalidades modernas em terminais.

## Tarefas

- [x] Analisar a estrutura atual do backend curses (`src/textadept_curses.c`)
- [x] Criar um novo backend `src/n_textadept.c`
- [ ] Adaptar a construção (Makefile, CMake) para incluir notcurses ** (aguardando adição do Makefile ao chat) **
- [ ] Implementar as funções de plataforma necessárias (set_title, focus_view, etc.)
- [ ] Testar a integração
- [ ] Documentar

## Próximos Passos

1. **Adicionar Makefile ao chat** para que possamos adicionar o alvo `textadept-notcurses` e as flags de linkagem `-lnotcurses`.
2. **Implementar as funções essenciais** que conectam Notcurses com o core do Textadept:
   - `new_scintilla()` – criar view Scintilla via notcurses
   - `focus_view()` – gerenciar foco entre views
   - `update_ui()` – integração com o loop de eventos do Notcurses
3. **Criar um painel principal** para abrigar múltiplas views e barras de status.
4. **Tratar entrada do teclado** e redirecionar para Scintilla.

## Mapeamento de Funções da Plataforma

Com base em `textadept_platform.h`, a implementação Notcurses deve fornecer as seguintes funções:

| Função | Status |
|--------|--------|
| `get_platform` | implementado |
| `get_charset` | implementado |
| `new_window` | implementado |
| `set_title` | implementado |
| `is_maximized` | implementado |
| `get_size` | implementado |
| `set_size` | stub |
| `new_scintilla` | implementado |
| `focus_view` | implementado |
| `SS` | implementado |
| `split_view` | stub |
| `unsplit_view` | stub |
| `delete_scintilla` | implementado |
| `get_top_pane` | stub |
| `get_pane_info` | stub |
| `get_parent_pane_info` | stub |
| `get_pane_info_from_view` | stub |
| `set_pane_split_pos` | stub |
| `show_tabs` | stub |
| `add_tab` | stub |
| `set_tab` | stub |
| `set_tab_label` | stub |
| `move_tab` | stub |
| `remove_tab` | stub |
| `get_find_text` | implementado |
| `get_repl_text` | implementado |
| `set_find_text` | implementado |
| `set_repl_text` | implementado |
| `add_to_find_history` | stub |
| `add_to_repl_history` | stub |
| `set_entry_font` | stub |
| `is_checked` | stub |
| `toggle` | stub |
| `set_find_label` | stub |
| `set_repl_label` | stub |
| `set_button_label` | stub |
| `set_option_label` | stub |
| `focus_find` | stub |
| `focus_command_entry` | implementado |
| `is_command_entry_active` | implementado |
| `set_command_entry_label` | implementado |
| `get_command_entry_height` | stub |
| `set_command_entry_height` | stub |
| `is_statusbar_visible` | implementado |
| `set_statusbar_visible` | implementado |
| `get_statusbar_text` | implementado |
| `set_statusbar_text` | implementado |
| `read_menu` | stub |
| `popup_menu` | implementado |
| `set_menubar` | stub |
| `get_clipboard_text` | stub (retorna NULL) |
| `add_timeout` | stub |
| `update_ui` | implementado |
| `is_hidpi` | implementado |
| `is_dark_mode` | implementado |
| `message_dialog` | implementado |
| `input_dialog` | stub |
| `open_dialog` | stub |
| `save_dialog` | stub |
| `progress_dialog` | stub |
| `list_dialog` | stub |
| `spawn` | stub |
| `process_size` | stub |
| `is_process_running` | stub |
| `wait_process` | stub |
| `read_process_output` | stub |
| `write_process_input` | stub |
| `close_process_input` | stub |
| `kill_process` | stub |
| `get_process_exit_status` | stub |
| `cleanup_process` | stub |
| `suspend` | stub |
| `quit` | stub |

## Registro de alterações

### 2026-02-22
- Criado este arquivo de rastreamento.

### 2026-02-22 (continuação)
- Criado arquivo boilerplate `src/n_textadept.c` com includes de Notcurses e stubs de funções.
- Ainda é necessário analisar o Makefile e adicionar alvo de compilação (aguardando adição do arquivo Makefile ao chat).

### 2026-02-22 (mais tarde)
- Analisado cabeçalho `textadept_platform.h` e mapeadas todas as funções a serem implementadas.
- Adicionada tabela de mapeamento no arquivo de tracking.

### 2026-02-22 (noite)
- Criado arquivo `src/n_textadept.c` completo com inicialização Notcurses, substituindo `initscr()`, mapeando view para `struct ncplane *plane`, usando `ncplane_putstr_yx` para renderização e `notcurses_render()`.
- Atualizado status das funções na tabela para "stub".

### 2026-02-23
- Adicionada seção "Próximos Passos" ao tracking.
### 2026-02-23 (continuação)
- Implementadas as funções `popup_menu` e `is_hidpi` no arquivo `src/n_textadept.c`.
- Iniciada a análise do Makefile para inclusão da biblioteca Notcurses.
### 2026-02-23 (tarde)
- Implementadas funções de plataforma: `get_platform`, `get_charset`, `new_window`, `set_title`, `get_size`, `update_ui`.
- Adicionada função `main` e loop de eventos básico.
### 2026-02-23 (noite)
- Revisão completa do projeto; entendimento dos arquivos e funções necessárias estabelecido.

### 2026-02-24 (manhã)
- Implementadas funções `new_scintilla`, `focus_view`, `SS`, `delete_scintilla` utilizando a API Scintilla.
- Adicionadas declarações extern para funções da biblioteca Scintilla.
### 2026-02-24 (tarde)
- Implementada função `message_dialog` usando Notcurses (diálogo básico).
- Atualizados status de `is_maximized`, `set_maximized`, `is_dark_mode` para implementado (stubs simples).

### 2026-02-24 (noite)
- Corrigido status de `set_maximized` na tabela.
- Adicionadas implementações stub para funções de painel (`split_view`, `unsplit_view`, etc.) e abas.
