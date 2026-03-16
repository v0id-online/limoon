# Textadept — Frontend Notcurses

Este documento explica a arquitetura, dependências, build e funcionamento interno
do fork do Textadept que usa **Notcurses** como backend de terminal, substituindo
o backend ncurses/CDK original.

---

## Índice

1. [Visão Geral](#1-visão-geral)
2. [Dependências](#2-dependências)
3. [Build](#3-build)
4. [Estrutura de Arquivos](#4-estrutura-de-arquivos)
5. [Arquitetura Interna](#5-arquitetura-interna)
6. [Sistema de Temas](#6-sistema-de-temas)
7. [Sistema de Plugins](#7-sistema-de-plugins)
8. [Aliases de Linha de Comando](#8-aliases-de-linha-de-comando)
9. [Referência de Teclas](#9-referência-de-teclas)
10. [Guia do Desenvolvedor](#10-guia-do-desenvolvedor)
11. [Limitações Conhecidas](#11-limitações-conhecidas)

---

## 1. Visão Geral

O Textadept original suporta dois frontends: **GTK** (GUI) e **curses** (terminal
via ncurses). Este fork substitui o backend curses por **Notcurses**, uma biblioteca
de terminal moderna que oferece:

- Cores true-color (24-bit RGB) em terminais compatíveis
- Renderização baseada em planos (`ncplane`) compostos em camadas
- API mais limpa e previsível que ncurses
- Suporte nativo a Unicode/UTF-8

O componente de edição continua sendo **Scintilla** — a mesma biblioteca usada
no editor GUI. A ponte entre Scintilla e Notcurses é feita pela biblioteca
**scinterm-notcurses** (submodule git).

```
┌─────────────────────────────────────────────────────┐
│               textadept-notcurses                   │
├─────────────────┬───────────────────────────────────┤
│  Core Lua/C     │  Frontend: src/n_textadept.c       │
│  (textadept     │  • loop de eventos Notcurses       │
│   original)     │  • renderização de UI (find bar,   │
│                 │    statusbar, diálogos)             │
│                 │  • despacho de teclado/mouse        │
├─────────────────┴───────────────────────────────────┤
│           scinterm-notcurses  (submodule)            │
│  • libscinterm_notcurses_static.a                   │
│  • adapta Scintilla para renderizar em ncplane      │
├─────────────────────────────────────────────────────┤
│  Notcurses   │   Scintilla   │   Lua 5.4            │
└─────────────────────────────────────────────────────┘
```

---

## 2. Dependências

### Dependências do sistema

| Pacote | Versão mínima | Para que serve |
|---|---|---|
| **notcurses** | 3.0 | Backend de terminal |
| **gcc** | com suporte C17 (`-std=gnu17`) | Compilação |
| **cmake** | 3.10 | Build do submodule scinterm-notcurses |
| **pkg-config** | qualquer | Localizar flags do notcurses |

#### Fedora / RHEL

```bash
sudo dnf install notcurses-devel cmake gcc gcc-c++ pkg-config
```

#### Debian / Ubuntu

```bash
sudo apt install libnotcurses-dev cmake gcc g++ pkg-config
```

#### Arch Linux

```bash
sudo pacman -S notcurses cmake gcc pkg-config
```

### Dependências embutidas (baixadas pelo CMake)

O build do core do Textadept usa CMake `FetchContent` para baixar e compilar
automaticamente:

| Biblioteca | Localização após build |
|---|---|
| Lua 5.4 | `build/_deps/lua-src/` |
| LPeg | `build/_deps/lpeg-src/` |
| LFS (LuaFileSystem) | `build/_deps/lfs-src/` |
| Regex (TRE) | `build/_deps/regex-src/` |

Estas já devem estar compiladas em `build/*.a` se o projeto foi clonado com
histórico completo. Se não estiverem, rode o build original do Textadept uma
vez (`cmake -S . -B build && cmake --build build`) antes de usar o Makefile
deste fork.

### Submodule: scinterm-notcurses

```
scinterm-notcurses/          ← git submodule
├── include/                 ← cabeçalhos (ScintillaNotCurses.h, etc.)
├── scintilla/               ← Scintilla fonte
│   └── include/             ← Scintilla.h, ScintillaTypes.h, etc.
└── build/                   ← gerado pelo cmake
    └── libscinterm_notcurses_static.a
```

---

## 3. Build

### Primeira vez (clone limpo)

```bash
# 1. Inicializar o submodule
git submodule update --init scinterm-notcurses

# 2. Compilar scinterm-notcurses (biblioteca estática)
cmake -S scinterm-notcurses -B scinterm-notcurses/build \
  -DBUILD_SHARED_LIBS=OFF \
  -DBUILD_EXAMPLES=OFF \
  -DBUILD_TESTING=OFF \
  -DENABLE_SCINTILLUA=OFF
cmake --build scinterm-notcurses/build --parallel

# 3. Garantir que as dependências Lua/LPeg/LFS estão compiladas
#    (necessário apenas uma vez; pula se build/*.a já existirem)
cmake -S . -B build
cmake --build build --target liblua liblpeg liblfs libregex

# 4. Compilar o executável
make
```

### Builds seguintes

```bash
make          # recompila apenas o que mudou
make clean    # remove objetos e executável (mantém scinterm-notcurses/build)
make clean-all  # remove tudo inclusive scinterm-notcurses/build
```

### Executar

```bash
./textadept-notcurses [arquivo]
```

---

## 4. Estrutura de Arquivos

```
textadept/
│
├── src/
│   ├── n_textadept.c        ★ Frontend principal (Notcurses)
│   ├── textadept.h          ★ Declarações do core modificadas
│   ├── textadept_platform.h ★ Interface de plataforma (funções que o
│   │                           frontend deve implementar)
│   └── textadept_curses.c   ← frontend ncurses original (não usado)
│
├── core/                    ← Lua core do Textadept (não modificado exceto)
│   ├── ui.lua               ★ statusbar, find bar, command entry
│   └── file_io.lua          ★ diálogo de saída (botão "Quit")
│
├── modules/                 ← Módulos Lua carregados por init.lua
│   ├── ui_widgets.lua       ★ Componentes reutilizáveis (statusbar, notify,
│   │                           dialogs, output buffers)
│   ├── themes.lua           ★ Gerenciador de temas
│   ├── plugin_manager.lua   ★ Carregamento automático de plugins
│   └── aliases.lua          ★ Atalhos de linha de comando
│
├── plugins/                 ← Plugins auto-carregados
│   ├── word_count.lua       statusbar W:N L:N  +  Ctrl+Shift+W
│   ├── git_status.lua       statusbar branch   +  Ctrl+Shift+G
│   ├── scratch_pad.lua      buffer de notas       Ctrl+Shift+N
│   └── help.lua             referência rápida     Ctrl+H
│
├── themes/                  ← Temas de cores
│   ├── _base.lua            ★ Aplicador base (chamado por cada tema)
│   ├── monokai.lua          ... e outros 19 temas comunitários
│   └── ...
│
├── init.lua                 ★ Inicialização principal (modificado)
├── Makefile                 ★ Build deste fork
├── scinterm-notcurses/      ← Submodule git
└── NOTCURSES.md             este arquivo
```

Arquivos marcados com ★ foram criados ou modificados neste fork.

---

## 5. Arquitetura Interna

### 5.1 n_textadept.c — o frontend

Este arquivo implementa todas as funções declaradas em `textadept_platform.h`.
É o equivalente Notcurses de `textadept_curses.c`.

#### Estruturas principais

```c
/* Uma view Scintilla renderizada em um ncplane */
struct SciPanel {
    ScintillaNotCurses *sci;   // instância Scintilla
    struct ncplane     *plane; // plano Notcurses onde é renderizada
    int row, col, rows, cols;  // posição e tamanho em células
};

/* Estado global do editor */
static struct notcurses    *nc;           // contexto Notcurses
static struct ncplane      *stdplane;     // plano raiz
static struct SciPanel     *current_view; // view com foco
static struct ncplane      *statusbar_plane;
static struct ncplane      *tabbar_plane;
```

#### Loop de eventos

```c
// Simplificado — veja main() em n_textadept.c
while (!should_quit) {
    update_ui();                      // atualiza statusbar, renderiza
    notcurses_render(nc);             // compõe e envia ao terminal
    notcurses_get_blocking(nc, &ni);  // aguarda próximo evento
    handle_keypress(&ni);             // despacha para Scintilla ou UI
}
```

#### Renderização da Scintilla

A Scintilla mantém estado interno e renderiza em seu `ncplane` quando chamada:

```c
scintilla_render(current_view->sci);
```

Esta chamada deve ocorrer **antes** de `notcurses_render()` para que a edição
fique visível. É crítico chamá-la também dentro de loops modais (find bar,
diálogos), pois nesses casos o loop principal fica bloqueado.

### 5.2 Find Bar (Ctrl+F)

A find bar é implementada inteiramente em C como um loop modal:

```
┌─ find bar (última linha da tela) ───────────────────────────────┐
│ Find: [________________] [◀Prev] [Next▶] [HiAll] [✕]           │
│ Repl: [________________] [Replace] [Replace All]                │
└─────────────────────────────────────────────────────────────────┘
```

**Navegação:** Tab/Shift+Tab circula entre os 8 elementos (2 entradas + 6 botões).
O botão com foco é desenhado com cores invertidas.

**Loop modal:**

```c
while (!done) {
    scintilla_render(current_view->sci);   // ← crítico: mostra resultados
    draw_findbar(...);
    notcurses_render(nc);
    notcurses_get_blocking(nc, &ni);
    // processa Tab, Enter, Esc, texto...
}
```

**Highlight de resultados:** configurado via `INDIC_ROUNDBOX` em `_base.lua`;
a cor vem de `colors.find` definida em cada tema.

**Seleção do texto encontrado:** usa `ELEMENT_SELECTION_BACK` com alpha `0xFF`.
O alpha é obrigatório pois `ColourRGBA::IsValid()` da Scintilla retorna `false`
quando o byte de alpha é zero, desabilitando o highlight.

### 5.3 Diálogos

Todos os diálogos são implementados como loops modais em C usando `ncplane`:

| Diálogo | Função C | Usa |
|---|---|---|
| Mensagem / Confirmação | `message_dialog()` | box-drawing chars, aceleradores sublinhados |
| Entrada de texto | `input_dialog()` | campo editável com cursor |
| Lista | `list_dialog()` | scroll, seleção por teclado |
| Abrir / Salvar arquivo | `open_dialog()` / `save_dialog()` | navegação de diretório |

O diálogo de saída (Ctrl+Q) exibe botões **S**ave / **C**ancel / **Q**uit com
a primeira letra sublinhada e em cor de acento. Pressionar a letra correspondente
ativa o botão diretamente.

### 5.4 Statusbar

A statusbar é composta via `ui_widgets.lua`. Plugins registram segmentos:

```lua
local W = require('ui_widgets')
W.status_add('meu_plugin', function() return 'texto' end, prioridade)
```

Menor prioridade = aparece mais à esquerda. Layout atual:

```
 branch*  │  W:1234  L:56  │  ? Ctrl+H
   (5)         (20)            (90)
```

---

## 6. Sistema de Temas

### Aplicar um tema

```lua
-- No command entry (Ctrl+;):
theme "monokai"    -- aplica imediatamente e salva preferência
themes()           -- abre seletor interativo
```

A preferência é salva em `~/.textadept/theme` e recarregada na próxima abertura.

### Estrutura de um tema

```lua
-- themes/meutema.lua
local view, colors, styles = view, view.colors, view.styles

-- Cores em formato 0xBBGGRR (Blue=byte alto, Green=meio, Red=byte baixo)
-- Para converter de #RRGGBB: inverta os bytes → 0xBBGGRR
colors.bg      = 0x222827 -- #272822 (fundo)
colors.fg      = 0xF2F8F8 -- #F8F8F2 (texto)
colors.comment = 0x5E7175 -- #75715E
colors.str     = 0x74DBE6 -- #E6DB74
colors.kw      = 0x7226F9 -- #F92672 (keywords)
colors.func    = 0x2EE2A6 -- #A6E22E
colors.num     = 0xFF81AE -- #AE81FF
colors.cls     = 0xE8D966 -- #66D9E8 (classes/tipos)
colors.builtin = 0xE8D966
colors.attr    = 0x2EE2A6
colors.err     = 0x2F00CC -- #CC002F
colors.sel     = 0x3E4849 -- #49483E (seleção de texto normal)
colors.find    = 0x1F97FD -- #FD971F (highlight do find / seleção de busca)
colors.cur     = 0x323D3E -- #3E3D32 (linha atual)
colors.lnum    = 0x5E7175 -- linha de número

dofile(_HOME .. '/themes/_base.lua')(view, colors, styles)
```

### Cor `colors.find`

`colors.find` é usada em dois lugares pelo `_base.lua`:

1. **`ELEMENT_SELECTION_BACK`** — cor de fundo da seleção (inclui texto
   encontrado pelo Ctrl+F). Aplicada com `| 0xFF000000` para garantir alpha opaco.

2. **`INDIC_FIND`** — indicador `INDIC_ROUNDBOX` do "Highlight All" com
   alpha semitransparente (100/255).

Para temas escuros recomenda-se um âmbar quente (ex: `#E8B44B` → `0x4BB4E8`).
Para temas claros, um laranja mais saturado (ex: `#FF8600` → `0x0086FF`).

### Adicionar um novo tema

1. Crie `themes/meutema.lua` seguindo o modelo acima.
2. Adicione o nome à lista `M.available` em `modules/themes.lua`.
3. Pronto — o seletor interativo (`themes()`) vai listá-lo.

---

## 7. Sistema de Plugins

### Como funciona

`modules/plugin_manager.lua` carrega automaticamente todos os arquivos `.lua`
de dois diretórios (na ordem):

1. `~/.textadept/plugins/` — plugins do usuário (sobrescrevem os built-in)
2. `_HOME/plugins/` — plugins built-in do editor

### Estrutura de um plugin

```lua
-- plugins/meu_plugin.lua
local W = require('ui_widgets')

-- Registrar segmento na statusbar (opcional)
W.status_add('meu_plugin', function()
  return 'info'
end, 50)  -- prioridade 50

-- Registrar atalho de teclado (opcional)
keys['ctrl+shift+m'] = function()
  local buf = W.output_buffer('[Meu Plugin]')
  W.write(buf, 'Olá mundo\n', true)
end

-- Metadados (exibidos pelo Ctrl+Shift+P)
return {
  _meta = {
    name        = 'Meu Plugin',
    description = 'Descrição curta',
    version     = '1.0',
    author      = 'Seu Nome',
  }
}
```

### API ui_widgets

```lua
local W = require('ui_widgets')

-- Statusbar
W.status_add(id, fn, priority)  -- registra segmento; fn() → string
W.status_remove(id)             -- remove segmento
W.notify(text, secs)            -- mensagem temporária

-- Diálogos
W.input(title, prompt, default) -- → string ou nil
W.confirm(title, msg)           -- → true/false
W.pick(title, items, columns)   -- → índice ou nil

-- Buffers de saída (somente-leitura, identificados por _type)
buf = W.output_buffer(name)     -- cria ou foca buffer nomeado
W.write(buf, text, clear)       -- escreve no buffer
```

### Plugins built-in

| Arquivo | Atalho | Descrição |
|---|---|---|
| `word_count.lua` | Ctrl+Shift+W | Contagem de palavras/linhas/chars |
| `git_status.lua` | Ctrl+Shift+G | Branch git na statusbar + painel de status |
| `scratch_pad.lua` | Ctrl+Shift+N | Buffer de notas persistente (`~/.textadept/scratch.txt`) |
| `help.lua` | Ctrl+H | Referência rápida somente-leitura |

---

## 8. Aliases de Linha de Comando

O command entry (`Ctrl+;`) aceita qualquer expressão Lua. O módulo
`modules/aliases.lua` define funções curtas no namespace global:

| Alias | Equivalente completo |
|---|---|
| `theme "nome"` | `textadept.themes.set("nome")` |
| `themes()` | `textadept.themes.select()` |
| `e "path"` | `io.open_file("path")` |
| `save()` | `buffer:save()` |
| `saveas "path"` | `buffer:save_as("path")` |
| `bclose()` | `buffer:close()` |
| `reload()` | `buffer:reload()` |
| `bnext()` / `bprev()` | `view:goto_buffer(1/−1)` |
| `ln(n)` | `textadept.editing.goto_line(n)` |
| `lex "nome"` | `buffer:set_lexer("nome")` |
| `wrap()` | toggle `view.wrap_mode` |
| `tabs(n)` | `buffer.use_tabs=true; buffer.tab_width=n` |
| `spaces(n)` | `buffer.use_tabs=false; buffer.tab_width=n` |
| `zoom(n)` | `buffer:zoom_in/out()` ou reset |
| `run()` | `textadept.run.run()` |
| `plugins()` | `textadept.plugins.show()` |
| `aliases()` | lista todos os aliases num buffer |

---

## 9. Referência de Teclas

### Arquivos e Buffers

| Tecla | Ação |
|---|---|
| Ctrl+O | Abrir arquivo |
| Ctrl+S | Salvar |
| Ctrl+Shift+S | Salvar como |
| Ctrl+W | Fechar buffer |
| Ctrl+N | Novo buffer |
| Ctrl+Tab | Próximo buffer |
| Ctrl+Shift+Tab | Buffer anterior |
| Ctrl+Q | Sair |

### Edição

| Tecla | Ação |
|---|---|
| Ctrl+Z / Ctrl+Y | Desfazer / Refazer |
| Ctrl+X / C / V | Recortar / Copiar / Colar |
| Ctrl+A | Selecionar tudo |
| Ctrl+D | Duplicar linha |
| Ctrl+/ | Comentar/descomentar |
| Tab / Shift+Tab | Indentar / Desdentar |

### Busca

| Tecla | Ação |
|---|---|
| Ctrl+F | Abrir/fechar find bar |
| Enter | Buscar próximo |
| Shift+Enter | Buscar anterior |
| Tab / Shift+Tab | Ciclar botões da find bar |
| Esc | Fechar find bar |

### Plugins e UI

| Tecla | Ação |
|---|---|
| Ctrl+H | Ajuda (este buffer, toggle) |
| Ctrl+Shift+N | Scratch Pad |
| Ctrl+Shift+G | Painel Git Status |
| Ctrl+Shift+W | Diálogo Word Count |
| Ctrl+Shift+P | Lista de plugins |
| Ctrl+; | Command entry (Lua) |

---

## 10. Guia do Desenvolvedor

### Onde adicionar uma nova função de plataforma

Todas as funções que o core do Textadept chama para interagir com a UI estão
declaradas em `src/textadept_platform.h`. A implementação deve estar em
`src/n_textadept.c`.

Exemplo — adicionar um indicador de modo na statusbar:

```c
// Em n_textadept.c, dentro de draw_statusbar():
ncplane_printf_yx(statusbar_plane, 0, col, " [%s]", insert_mode ? "INS" : "OVR");
```

### Criar um diálogo modal

O padrão usado neste fork é:

```c
static int meu_dialogo(const char *titulo, ...) {
    // 1. criar ncplane sobre o stdplane
    struct ncplane_options opts = { .y=y, .x=x, .rows=h, .cols=w };
    struct ncplane *dp = ncplane_create(stdplane, &opts);

    // 2. desenhar borda e conteúdo
    draw_border(dp, ...);

    // 3. loop modal
    bool done = false;
    int result = 0;
    while (!done) {
        notcurses_render(nc);
        struct ncinput ni;
        notcurses_get_blocking(nc, &ni);
        // processar ni.id (tecla), ni.modifiers, etc.
        if (ni.id == NCKEY_ENTER) { result = 1; done = true; }
        if (ni.id == NCKEY_ESC)   { result = 0; done = true; }
    }

    ncplane_destroy(dp);
    return result;
}
```

### Cores nos planos Notcurses

```c
// Definir foreground/background em um ncplane
ncplane_set_fg_rgb8(plane, r, g, b);
ncplane_set_bg_rgb8(plane, r, g, b);

// Escrever com estilo
ncplane_set_styles(plane, NCSTYLE_BOLD | NCSTYLE_UNDERLINE);
ncplane_putstr_yx(plane, row, col, "texto");
ncplane_set_styles(plane, NCSTYLE_NONE);
```

### Formato de cores nos temas Lua

O Textadept e a Scintilla usam o formato `0xBBGGRR` internamente (diferente
do `0xRRGGBB` usual):

```lua
-- #RRGGBB → 0xBBGGRR: trocar a ordem dos bytes
-- Exemplo: #FF8600 (laranja) → R=FF G=86 B=00 → 0x0086FF
colors.find = 0x0086FF  -- exibe como laranja #FF8600
```

Para `ELEMENT_SELECTION_BACK` é necessário o OR com `0xFF000000` para que o
byte de alpha seja 0xFF (opaco), caso contrário `ColourRGBA::IsValid()` retorna
`false` e o highlight não aparece:

```lua
view.element_color[view.ELEMENT_SELECTION_BACK] = colors.find | 0xFF000000
```

### Scintilla via SCI_* messages

A comunicação com a Scintilla é feita por `SS()` (Send Scintilla):

```c
// Em C:
sptr_t resultado = SS(view, SCI_GETLENGTH, 0, 0);
SS(view, SCI_GOTOPOS, posicao, 0);

// O tipo ScintillaNotCurses é opaco; use sempre SS() ou a API C++ interna
```

---

## 11. Limitações Conhecidas

| Área | Limitação |
|---|---|
| **Split de views** | `split_view` / `unsplit_view` são stubs; múltiplas views ainda não renderizam corretamente lado a lado |
| **Mouse** | Eventos de mouse são recebidos mas o suporte é parcial (cliques na find bar funcionam; drag de seleção pode ser inconsistente) |
| **Processos externos** | `spawn()` usa `fork/exec` básico; pipes de leitura assíncrona (`read_process_output`) são bloqueantes |
| **Diálogo de arquivo** | `open_dialog` / `save_dialog` mostram campo de texto simples; não há navegação de diretório com lista |
| **Redimensionamento** | `SIGWINCH` (redimensionamento do terminal) não é tratado; reinicie o editor se redimensionar a janela |
| **Clipboard** | Usa seleção X11 (`xclip`/`xsel`) quando disponível; em terminais sem X, o clipboard é interno ao processo |
| **Scrollbar** | Não renderizada; scroll funciona via teclado e roda do mouse |

---

*Última atualização: 2026-03-15*
