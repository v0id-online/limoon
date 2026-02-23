# Notcurses Implementation Tracking

Data de início: 2026-02-22

Este arquivo rastreia as alterações e progresso da implementação do Textadept usando Notcurses.

## Objetivo

Substituir a interface baseada em ncurses (atualmente via CDK?) pela biblioteca Notcurses para melhor suporte a gráficos, cores, e funcionalidades modernas em terminais.

## Tarefas

- [ ] Analisar a estrutura atual do backend curses (`src/textadept_curses.c`)
- [ ] Criar um novo backend `src/textadept_notcurses.c`
- [ ] Adaptar a construção (Makefile, CMake) para incluir notcurses
- [ ] Implementar as funções de plataforma necessárias (set_title, focus_view, etc.)
- [ ] Testar a integração
- [ ] Documentar

## Registro de alterações

### 2026-02-22
- Criado este arquivo de rastreamento.
