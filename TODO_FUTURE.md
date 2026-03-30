# Li Moon - TODOs e Melhorias Futuras

> Arquivo gerado automaticamente com base na varredura de bugs do projeto.
> Data: 2026-03-28

---

## 🐛 Bugs Conhecidos

### Core

#### file_io_test.lua
- **Linha 255**: Teste ignorado no macOS devido a erro de sistema no iconv
  ```lua
  if OSX then skip('crashes on macOS due to system iconv error') end
  ```

- **Linha 717**: Handler de `events.BUFFER_DELETED` assume que o buffer fechado estava focado
  ```lua
  expected_failure() -- TODO: ui.lua's events.BUFFER_DELETED handler assumes closed buffer was focused
  ```

#### init_test.lua
- **Linha 123**: Teste de quit incompleto
  ```lua
  -- TODO: quit?
  ```

#### ui_test.lua
- **Linha 545**: Problema com splits[2].size[3] == 0 (apenas GTK2 funciona)
  ```lua
  if not gtk2 then expected_failure() end -- TODO: splits[2].size[3] == 0
  ```

- **Linha 583**: Teste esperando falha (motivo não especificado)
  ```lua
  expected_failure() -- TODO:
  ```

- **Linha 595**: Popup menu não implementado em testes
  ```lua
  -- TODO: ui.popup_menu
  ```

#### view_test.lua
- **Linha 38**: Falha esperada
  ```lua
  expected_failure() -- TODO:
  ```

- **Linha 73**: Problema com second size[3] == 0 (apenas GTK2 funciona)
  ```lua
  if not gtk2 then expected_failure() end -- TODO: second size[3] == 0
  ```

#### ui_dialogs_test.lua
- **Linha 16**: Testes de dialog causam problemas de foco no macOS
  ```lua
  if OSX then skip('this test appears to cause focus issues for command entry tests') end
  ```

- **Linha 31**: Mocking do restante parece sem sentido
  ```lua
  -- TODO: mocking the rest of these seems pointless.
  ```

---

## ⏳ Funcionalidades Não Implementadas

### Menu/Comandos (modules/limoon/keys.lua)

| Linha | Funcionalidade | Descrição |
|-------|----------------|-----------|
| 361 | `io.open_recent_file` | Abrir arquivo recente |
| 362 | `buffer.reload` | Recarregar buffer |
| 365 | `io.save_all_files` | Salvar todos os arquivos |
| 368 | `limoon.sessions.load` | Carregar sessão |
| 369 | `limoon.sessions.save` | Salvar sessão |
| 413 | `limoon.history.record` | Gravar histórico de navegação |
| 414 | `limoon.history.clear` | Limpar histórico de navegação |
| 420 | `ui.find.find_next` | Próxima ocorrência (via menu) |
| 421 | `ui.find.find_prev` | Ocorrência anterior (via menu) |
| 422 | `ui.find.replace` | Substituir (via menu) |
| 423 | `ui.find.replace_all` | Substituir tudo (via menu) |
| 451 | `limoon.bookmarks.clear` | Limpar bookmarks |
| 458 | `limoon.macros.save` | Salvar macro gravada |
| 459 | `limoon.macros.load` | Carregar macro salva |
| 463 | `Quick Open Current Directory` | Abrir diretório atual rapidamente |
| 466 | `limoon.snippets.select` | Selecionar snippet |
| 470 | `Complete Trigger Word` | Completar palavra trigger |
| 473 | `Tools/Show Style` | Mostrar estilo no cursor |
| 483-485 | Tab width presets | Larguras de tab pré-definidas (2, 3, 4, 8) |

### Session (modules/limoon/session.lua)

- **Linha 86**: Revisar `split_pos` - pode estar incorreto
  ```lua
  if i == 1 then view.split_pos = split.size end -- TODO: split_pos?
  ```

- **Linha 172 (teste)**: Falha esperada
  ```lua
  expected_failure() -- TODO:
  ```

### Snippets (modules/limoon/snippets.lua)

- **Linha 673**: Inserir transformação inicial quando `ph.index > self.index`
  ```lua
  -- TODO: insert initial transform for ph.index > self.index
  ```

### Run (modules/limoon/run_test.lua)

- **Linha 381**: Lexer de output não reconhece paths absolutos no Windows (C:\)
  ```lua
  if WIN32 then expected_failure() end -- TODO: output lexer does not recognize absolute c:\ paths
  ```

### Keys (modules/limoon/keys_test.lua)

- **Linha 36**: Teste falha aleatoriamente
  ```lua
  skip('this test randomly fails') -- TODO: no amount of ui.update() is good enough
  ```

---

## 🏗️ Limitações de Arquitetura

### Split View (C)

**Arquivo**: `src/n_limoon.c:1746`

```c
/* ------------------------------------------------------------------ */
/* Split/pane (Bug H — stub: proper pane tree requires significant work) */
```

- A árvore de panes é uma implementação stub
- Requer trabalho significativo para funcionar corretamente
- Problemas conhecidos:
  - Redimensionamento de panes pode não funcionar corretamente
  - Foco entre views split pode ser inconsistente
  - Unsplit pode não restaurar o layout correto

### Lexer (C/Lua)

**Arquivo**: `core/lexer_test.lua:222`

```lua
-- TODO: test buffer/scintilla <-> lexer api
```

- API entre buffer/Scintilla e lexer precisa de mais testes

---

## 📝 Melhorias de Documentação

### Keys (core/keys.lua)

- **Linha 48**: Documentar códigos de tecla não reconhecidos
  ```lua
  -- a trailing "0x*XXXX*", that number can be aliased to a string representation in `keys.KEYSYMS`.
  ```

---

## 🔧 Problemas de Compilação/Warning

### C - Warnings (não críticos)

| Arquivo | Linha | Descrição |
|---------|-------|-----------|
| `n_limoon.c` | 2099 | Retorno de `luaL_dostring` não usado |
| `n_limoon.c` | 3468-3507 | Possível truncation em snprintf do file browser |

---

## 💡 Sugestões de Melhoria

### 1. Sistema de Plugins
- Melhorar tratamento de erros em plugins
- Adicionar hot-reload de plugins
- Sistema de dependências entre plugins

### 2. Terminal/Notcurses
- Suporte completo a mouse
- Redimensionamento de terminal (SIGWINCH)
- Suporte a clipboard nativo do terminal

### 3. LSP/DAP (Planejado)
- Integração com Language Server Protocol
- Debug Adapter Protocol
- Tree-sitter para syntax highlighting

### 4. Git
- Integração com libgit2 (ao invés de shell out)
- Diff visual inline
- Blame annotations

### 5. Performance
- Lazy loading de módulos grandes
- Otimização de renderização para arquivos grandes
- Cache de syntax highlighting

---

## ✅ Bugs Corrigidos (Histórico)

### 2026-03-28

**Erros de sintaxe Lua (variadic arguments):**

1. `init.lua:34` - `function(...args)` → `function(...)`
2. `core/ui.lua:46` - `function(...args)` → `function(...)`
3. `core/ui.lua:133` - `function(...args)` → `function(...)`
4. `modules/limoon/command_entry.lua:180` - `function(...args)` → `function(...)`
5. `modules/limoon/snippets.lua:635` - `function(...captures)` → `function(...)`
6. `test/helpers.lua:73` - `function(...args)` → `function(...)`
7. `test/helpers.lua:102` - `function(...returns)` → `function(...)`

---

## 🎯 Prioridades

### Alta
1. Corrigir split view (Bug H)
2. Implementar LSP básico
3. Melhorar tratamento de erros nos plugins

### Média
4. Implementar comandos TODO do menu
5. Adicionar testes para lexer API
6. Melhorar suporte a mouse

### Baixa
7. Presets de tab width
8. Save/load de macros
9. Melhorias de performance

---

*Última atualização: 2026-03-28*
*Gerado por: Varredura de bugs do Kimi*
