/* Wrapper para fornecer o símbolo luaopen_scintillua que falta.
 * Inicializa o LPEG e retorna uma tabela vazia para o Scintillua.
 */

#include <lua.h>
#include <lauxlib.h>

/* Declaração da função luaopen_lpeg, fornecida pela libscintillua.so */
int luaopen_lpeg(lua_State *L);

int luaopen_scintillua(lua_State *L) {
    /* Inicializa o LPEG e coloca sua tabela na pilha */
    luaopen_lpeg(L);
    /* Remove a tabela do LPEG da pilha (deixando-a para o require) */
    lua_pop(L, 1);

    /* Cria uma nova tabela para o módulo scintillua.
       Aqui deveriam ser adicionadas funções, mas não temos.
       Retorna uma tabela vazia por enquanto. */
    lua_newtable(L);
    return 1;
}
