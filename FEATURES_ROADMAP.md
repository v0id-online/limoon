# Li Moon - Roadmap de Funcionalidades

> Funcionalidades priorizadas para implementação
> Data: 2026-03-28

---

## 🔥 PRIORIDADE 1 - Implementar Imediatamente

### 1. Seleção por Blocos (Block Selection)
**Descrição:** Selecionar texto em formato de bloco/coluna (rectangular selection)  
**Atalhos:**
- `Alt+Shift+Setas` - Estender seleção retangular
- `Ctrl+Shift+B` - Iniciar seleção por bloco no modo normal
- Com mouse: `Alt+Click+Drag`

**Uso:**
- Editar múltiplas linhas simultaneamente
- Selecionar colunas de dados
- Comentar/descomentar blocos de código

**Implementação:** Usar Scintilla's rectangular selection API (`SCI_SETSELECTIONMODE`, `SCI_SETRECTANGULARSELECTIONMODE`)

---

### 2. Auto-pairs (Smart Brackets)
**Descrição:** Auto-completar pares de caracteres  
**Caracteres:** `()`, `{}`, `[]`, `""`, `''`, `` `` ``

**Comportamento:**
- Digita `(` → insere `()` e posiciona cursor dentro
- Digita `)` quando já existe `)` → apenas move cursor para fora
- Seleção + `(` → envolve seleção em parênteses
- Backspace em `()` vazio → remove ambos

**Configuração por linguagem:**
```lua
autopairs.lua = { '(', ')', '{', '}', '[', ']', '"', "'" }
autopairs.rust = { '(', ')', '{', '}', '[', ']', '"', "'", '<', '>' }
```

---

### 3. Fuzzy Finder de Arquivos (Ctrl+Shift+F)
**Descrição:** Busca rápida de arquivos no projeto  
**Comando:** `fzf()` ou atalho `Ctrl+Shift+F`

**Funcionalidades:**
- Busca fuzzy (não precisa digitar exatamente)
- Preview do arquivo selecionado
- Ignora arquivos em `.gitignore`
- Mostra ícone por tipo de arquivo

**Interface:**
```
┌─────────────────────────────────────────┐
│ > config                                │
├─────────────────────────────────────────┤
│   src/config.lua              [lua]     │
│   core/config.lua             [lua]     │
│   test/config_test.lua        [lua]     │
│   docs/config.md              [md]      │
└─────────────────────────────────────────┘
```

---

### 4. Highlight de Palavra Sob Cursor
**Descrição:** Destacar todas as ocorrências da palavra atual  
**Atalho:** Automático ao posicionar cursor

**Comportamento:**
- Cursor em "function" → todas as "function" no arquivo são destacadas
- Cor diferente da seleção (ex: amarelo suave)
- Ignora case parcialmente (configurável)

**API:** Usar indicadores do Scintilla (`SCI_INDICATORFILLRANGE`)

---

## ⚡ PRIORIDADE 2 - Implementar em Seguida

### 5. Terminal Integrado (Ctrl+`)
**Descrição:** Terminal embutido no editor  
**Comandos:**
```lua
term()              -- Abre terminal na parte inferior
term("horizontal")  -- Split horizontal
term("vertical")    -- Split vertical
term.close()        -- Fecha terminal
```

**Funcionalidades:**
- Múltiplas abas de terminal
- Preserva histórico entre sessões
- Integração com variáveis de ambiente do projeto

---

### 6. Indent Guides
**Descrição:** Linhas verticais mostrando níveis de indentação  
**Visual:**
```lua
function example()
│   if true then
│   │   print("hello")
│   │   -- linha guia aqui
│   end
end
```

**Configuração:**
```lua
view.indent_guide_color = 0x555555
view.indent_guide_style = "dotted"  -- ou "solid", "dashed"
```

---

### 7. Relative Line Numbers
**Descrição:** Números de linha relativos à posição atual  
```
5  -- código
4  -- código
3  -- código
2  -- código
1  -- código
0  cursor aqui  <-- linha atual
1  -- código
2  -- código
3  -- código
```

**Facilita:** Movimentos tipo `5j`, `10k` no Vim mode

---

### 8. Auto-save
**Descrição:** Salvar automaticamente arquivos  
**Comando:** `autosave(seconds)`

**Opções:**
```lua
autosave(3)           -- Salvar após 3 segundos de inatividade
autosave("focus_lost") -- Salvar ao perder foco
autosave("change")     -- Salvar a cada alteração (com debounce)
```

---

## 📋 PRIORIDADE 3 - Features Avançadas

### 9. Git Gutter
**Descrição:** Indicadores de alterações git na margem  
**Símbolos:**
- `▐` linha modificada (amarelo)
- `▐` linha adicionada (verde)
- `▐` linha removida (vermelho)

**Posição:** Margem esquerda (ou overlay nos números de linha)

---

### 10. Workspace/Sessão Persistente
**Descrição:** Salvar e restaurar sessões de trabalho  
**Comandos:**
```lua
workspace.save("projeto-x")  -- Salva workspace atual
workspace.load("projeto-x")  -- Restaura workspace
workspace.list()             -- Lista workspaces salvos
```

**Persiste:**
- Arquivos abertos
- Posições de cursor
- Splits e suas configurações
- Histórico de comandos

---

### 11. Bookmarks Persistentes
**Descrição:** Marcar posições no código  
**Atalhos:**
```
Ctrl+Shift+M      -- Marcar/desmarcar posição atual
Ctrl+,            -- Próximo bookmark
Ctrl+.            -- Bookmark anterior
```

**Visual:** Ícone na margem (ex: 🔖)

---

### 12. Rainbow Parentheses
**Descrição:** Cores diferentes por nível de nesting  
```lua
function()           -- ( azul
  return function()  -- ( verde
    if true then     -- ( amarelo
      print("x")     
    end              -- ) amarelo
  end                -- ) verde
end                  -- ) azul
```

---

## 🛠️ Notas de Implementação

### Block Selection (PRIORIDADE 1)
**Arquivos envolvidos:**
- `src/n_limoon.c` - Adicionar tratamento de Alt+Shift+mouse
- `core/view.lua` - Expor funções de seleção retangular
- `init.lua` - Configurar keybindings

**Referência Scintilla:**
```c
SCI_SETRECTANGULARSELECTIONANCHOR(pos)
SCI_SETRECTANGULARSELECTIONCARET(pos)
```

### Auto-pairs
**Arquivo:** `modules/autopairs.lua` (novo)  
**Hook:** `events.KEY` para interceptar caracteres digitados

### Fuzzy Finder
**Arquivo:** `modules/fuzzy_finder.lua` (novo)  
**Dependência:** `find` ou `fd` (ferramenta externa)  
**UI:** Reusar `ui.dialogs.list` ou criar custom

### Highlight Palavra
**Arquivo:** `modules/word_highlight.lua` (novo)  
**Evento:** `events.UPDATE_UI`  
**Delay:** 300ms debounce para não laggear

---

## ✅ Checklist de Implementação

- [ ] Seleção por Blocos (Block Selection)
- [ ] Auto-pairs (Smart Brackets)
- [ ] Fuzzy Finder de Arquivos
- [ ] Highlight de Palavra Sob Cursor
- [ ] Terminal Integrado
- [ ] Indent Guides
- [ ] Relative Line Numbers
- [ ] Auto-save
- [ ] Git Gutter
- [ ] Workspace Persistente
- [ ] Bookmarks Persistentes
- [ ] Rainbow Parentheses

---

## 🎯 Próximos Passos

1. **Escolher qual implementar primeiro**
2. **Criar branch** para a feature
3. **Testar** em diferentes cenários
4. **Documentar** no help (Ctrl+H)
5. **Adicionar** ao CHANGELOG

---

*Atualizado: 2026-03-28*
