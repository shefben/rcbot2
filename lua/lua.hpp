#ifndef LUA_HPP
#define LUA_HPP

// This is a conceptual placeholder for the actual Lua headers.
// In a real build environment, you would have the Lua development libraries installed.

#ifdef __cplusplus
extern "C" {
#endif

#include <stdarg.h>
#include <stddef.h> // For size_t

// Basic types
#define LUA_TNONE		(-1)
#define LUA_TNIL		0
#define LUA_TBOOLEAN		1
#define LUA_TLIGHTUSERDATA	2
#define LUA_TNUMBER		3
#define LUA_TSTRING		4
#define LUA_TTABLE		5
#define LUA_TFUNCTION		6
#define LUA_TUSERDATA		7
#define LUA_TTHREAD		8

typedef double lua_Number;
typedef ptrdiff_t lua_Integer; // Changed from longlong to ptrdiff_t for wider compatibility
typedef unsigned int lua_Unsigned; // Changed from unsigned longlong
typedef struct lua_State lua_State;
typedef int (*lua_CFunction) (lua_State *L);

// State manipulation
lua_State *(luaL_newstate)(void);
void       (lua_close)(lua_State *L);
void       (luaL_openlibs)(lua_State *L);

// Stack manipulation
void       (lua_pushcclosure)(lua_State *L, lua_CFunction fn, int n);
void       (lua_setglobal)(lua_State *L, const char *name);
const char*(luaL_checklstring)(lua_State *L, int arg, size_t *l);
lua_Integer (luaL_checkinteger)(lua_State *L, int arg);
lua_Number  (luaL_checknumber)(lua_State *L, int arg);
void       (lua_pushstring)(lua_State *L, const char *s);
void       (lua_pushinteger)(lua_State *L, lua_Integer n);
void       (lua_pushnumber)(lua_State *L, lua_Number n);
void       (lua_pushnil)(lua_State *L);
void       (lua_newtable)(lua_State *L);
void       (lua_setfield)(lua_State *L, int idx, const char *k);
void       (lua_settable)(lua_State *L, int idx);
int        (lua_gettop)(lua_State *L);
void       (lua_pop)(lua_State *L, int n);
const char*(lua_tostring)(lua_State *L, int idx);


// Running Lua code
#define LUA_OK		0
#define LUA_ERRRUN	2
#define LUA_ERRSYNTAX	3
#define LUA_ERRMEM	4
#define LUA_ERRERR	5

int (luaL_dofile)(lua_State *L, const char *filename);
int (luaL_loadfile)(lua_State *L, const char *filename); // luaL_loadfilex for mode
int (lua_pcall)(lua_State *L, int nargs, int nresults, int msgh); // lua_pcallk for context

// Argument checking
#define luaL_checkstring(L, arg) luaL_checklstring(L, (arg), NULL)

// Helper for registering functions
#define lua_register(L,n,f) \
	(lua_pushcclosure(L, (f), 0), lua_setglobal(L, (n)))


#ifdef __cplusplus
}
#endif

#endif // LUA_HPP
