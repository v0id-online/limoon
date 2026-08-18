# Li Moon — i18n Audit: Portuguese Content Inventory

Scope: full repository tree, excluding `.git/`, `build/`, and the
`scinterm-notcurses` submodule (uninitialized, nothing checked out).
Read-only scan — no files were modified to produce this report.

Methodology: recursive search for accented Latin characters
(`áéíóúãõâêôç` + uppercase) plus accent-stripped Portuguese tokens
(`nao`, `arquivo`, `diretorio`, `instalacao`, `configuracao`, etc.) across
every text file extension in the repo, followed by manual inspection of
every hit to rule out false positives.

## Findings by category

### Comments (risk: LOW)

| File | Lines | Snippet |
|---|---|---|
| `src/lua_wrapper.c` | 1–22 (whole file) | `/* Wrapper para fornecer o símbolo luaopen_scintillua que falta. ... */` |
| `src/limoon.c` | 875 | `// Ajustar package.cpath e package.path para incluir caminhos do sistema Lua 5.4` |
| `src/n_limoon.c` | 139 | `/* Green — limão (default) */` |
| `src/n_limoon.c` | 1195–1196 | `/* Kitty protocol envia Ctrl+letra como uppercase. Normalizar para lowercase. ... */` |
| `install.sh` | 131 | `# Se for um link simbolico, copia o conteudo real` |
| `install.sh` | 142 | `# Importante: o binario precisa do LIMOON_HOME apontando para o diretorio de instalacao` |

### String literals (risk: MED)

None found. No Portuguese text appears in any code-level string literal
(dialog text, status messages, error strings) anywhere in the scanned
tree.

### Identifiers (risk: HIGH)

None found. No function, variable, or file name in the codebase uses
Portuguese vocabulary.

### Docs / markdown (risk: LOW)

| File | Lines | Notes |
|---|---|---|
| `FEATURES_ROADMAP.md` | 1–265 (whole file) | Feature-planning doc, 100% Portuguese prose. |
| `TODO_FUTURE.md` | 1–247 (whole file) | Backlog/TODO doc, 100% Portuguese prose. |
| `notcurses-track.md` | 1–209 (whole file) | Dev log tracking the Notcurses port, 100% Portuguese prose. |

## Confirmed false positives (excluded)

| File | Why excluded |
|---|---|
| `core/locales/locale.pt_BR.conf` | Intentional Brazilian Portuguese UI localization file — by design, not a leftover. |
| `core/locales/locale.es.conf`, `locale.fr.conf`, `locale.pl.conf`, `locale.sv.conf` | Other intentional locale files; accents belong to those languages. |
| `themes/rose-pine.lua:1` | "Rosé" is the upstream theme's French-derived name, not Portuguese content. |
| `docs/thanks.md:25,26,41` | Contributor proper names with diacritics, not Portuguese prose. |
| `docs/assets/images/*.png` | Binary bytes coincidentally matching the accent regex. |
| Various `error`/`show_error`/`goto_error` matches | False positives from the English substring "err" + "o"; all verified as legitimate English identifiers. |

## Areas scanned and clean

`core/*.lua` (incl. tests), `modules/`, `modules/limoon/*.lua` (incl.
tests), `plugins/*.lua`, `themes/*.lua` (except noted false positive),
`test/*.lua`, `src/limoon.h`, `src/limoon_platform.h`, `src/*.patch`,
root docs (`README.md`, `RELEASE.md`, `NOTCURSES.md`), `docs/api.md`,
`docs/changelog.md`, `docs/faq.md`, `docs/manual.md`,
`docs/_includes/head-custom.html`, build/config files (`CMakeLists.txt`,
`Makefile`, `.config.ld`, `.luacheckrc`, `.lua-lsp`, `.lua-format`,
`.luacov`, `.clang-format`, `.gitmodules`, `.gitignore`),
`.github/workflows/release.yml`, `install-binary.sh`, `fix-install.sh`,
`scripts/*`.

## Summary

| Category | Files | Risk |
|---|---|---|
| Comments | 4 files (`lua_wrapper.c`, `limoon.c`, `n_limoon.c`, `install.sh`), ~8 comment blocks | LOW |
| String literals | 0 | — |
| Identifiers | 0 | — |
| Docs/markdown | 3 files (whole-file PT) | LOW |

**Total files requiring remediation: 7** — all LOW risk. No user-facing
strings and no identifiers need translation, which removes the need for
Prompt 3 (string translation) and Prompt 4 (identifier rename) as
originally scoped; only Prompt 1 (docs) and Prompt 2 (comments) have real
work to do.

By directory:

| Directory | PT files |
|---|---|
| root | `FEATURES_ROADMAP.md`, `TODO_FUTURE.md`, `notcurses-track.md`, `install.sh` |
| `src/` | `lua_wrapper.c`, `limoon.c`, `n_limoon.c` |
| `core/`, `modules/`, `plugins/`, `themes/`, `docs/` | none |
