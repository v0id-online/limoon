/* Wrapper providing the missing luaopen_scintillua symbol.
 * Initializes LPeg and returns an empty table for Scintillua.
 */

#include <lua.h>
#include <lauxlib.h>

/* Declaration of luaopen_lpeg, provided by libscintillua.so */
int luaopen_lpeg(lua_State *L);

int luaopen_scintillua(lua_State *L) {
    /* Initialize LPeg and push its table onto the stack */
    luaopen_lpeg(L);
    /* Pop the LPeg table off the stack (leaving it for require) */
    lua_pop(L, 1);

    /* Create a new table for the scintillua module.
       Functions should be added here, but we have none.
       Returns an empty table for now. */
    lua_newtable(L);
    return 1;
}
