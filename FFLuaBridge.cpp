#include "FFLuaBridge.h"
#include "CRCBotPlugin.h"     // For g_pRCBotPlugin_Instance to access plugin members if needed
#include "EngineInterfaces.h" // For g_pEngineServer, g_pGlobals
#include "FFStateStructs.h"   // For Vector, etc. (though not directly used in these specific Lua funcs)

// Lua headers
extern "C" {
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
}

#include <iostream>

// --- Conceptual CGlobalVarsBase if not fully defined via EngineInterfaces.h ---
// This structure is typically provided by the engine.
#ifndef CGLOBALVARSBASE_CONCEPTUAL_DEF_FFLUABRIDGE
#define CGLOBALVARSBASE_CONCEPTUAL_DEF_FFLUABRIDGE
struct CGlobalVarsBase {
    float curtime;            // Current game time in seconds
    float frametime;          // Time elapsed since last frame
    int maxClients;         // Max number of clients on the server
    char mapname[128];      // Current map name
    // ... other fields like tickcount, interval_per_tick
    CGlobalVarsBase() : curtime(0.0f), frametime(0.015f), maxClients(32) { mapname[0] = '\0'; strncpy(mapname, "dummy_map", 127);}
};
// g_pGlobals is declared extern in EngineInterfaces.h and defined in CRCBotPlugin.cpp
#endif
// --- End Conceptual ---


namespace FFLuaBridge {

// --- Helper Function Implementations (from Task 13, Step 2 - ensure they are complete) ---
std::string SafeGetStringField(lua_State* L, int tableIndex, const char* fieldName, const char* defaultValue) {
    lua_getfield(L, tableIndex, fieldName);
    const char* val = luaL_optstring(L, -1, defaultValue);
    lua_pop(L, 1);
    return std::string(val);
}
int SafeGetIntegerField(lua_State* L, int tableIndex, const char* fieldName, int defaultValue) {
    lua_getfield(L, tableIndex, fieldName);
    int val = (int)luaL_optinteger(L, -1, defaultValue);
    lua_pop(L, 1);
    return val;
}
double SafeGetNumberField(lua_State* L, int tableIndex, const char* fieldName, double defaultValue) {
    lua_getfield(L, tableIndex, fieldName);
    double val = luaL_optnumber(L, -1, defaultValue);
    lua_pop(L, 1);
    return val;
}
bool SafeGetBooleanField(lua_State* L, int tableIndex, const char* fieldName, bool defaultValue) {
    lua_getfield(L, tableIndex, fieldName);
    bool val = defaultValue;
    if (lua_isboolean(L, -1)) { val = lua_toboolean(L, -1); }
    // else if (!lua_isnil(L, -1)) { /* Optional: treat non-nil as true, or error */ }
    lua_pop(L, 1);
    return val;
}
bool LuaTableToVector(lua_State* L, int tableIndex, Vector& vec) {
    if (!lua_istable(L, tableIndex)) { return false; }
    int absTableIndex = lua_absindex(L, tableIndex); // Ensure positive stack index
    vec.x = (float)SafeGetNumberField(L, absTableIndex, "x", vec.x);
    vec.y = (float)SafeGetNumberField(L, absTableIndex, "y", vec.y);
    vec.z = (float)SafeGetNumberField(L, absTableIndex, "z", vec.z);
    return true;
}

// --- Main Population Function Implementations (from Task 13, Step 2 - ensure they are complete) ---
bool PopulateControlPointInfo(lua_State* L, const char* gn, ControlPointInfo& ci) { /* ... from Task 13 ... */ return true;}
bool PopulatePayloadWaypoint(lua_State* L, int ti, PayloadWaypoint& wp) { /* ... from Task 13 ... */ return true;}
bool PopulatePayloadPathInfo(lua_State* L, const char* gn, PayloadPathInfo& pi) { /* ... from Task 13 ... */ return true;}
bool PopulateClassWeaponInfo(lua_State* L, int ti, ClassWeaponInfo& wi) { /* ... from Task 13 ... */ return true;}
bool PopulateClassConfigInfo(lua_State* L, const char* gn, ClassConfigInfo& ci) { /* ... from Task 13 ... */ return true;}
// --- End Population Stubs ---


// --- C++ Functions to be Exposed to Lua (Refined) ---

int Lua_RCBot_LogMessage(lua_State* L) {
    const char* message = luaL_checkstring(L, 1);
    if (message) {
        if (g_pEngineServer) {
            // g_pEngineServer->ConMsg("[RCBot Lua]: %s\n", message); // Conceptual direct engine call
            printf("[RCBot Lua via g_pEngineServer]: %s\n", message); // Placeholder using printf
        } else if (g_pRCBotPlugin_Instance) {
             g_pRCBotPlugin_Instance->LogMessageToConsole(message); // Assumes CRCBotPlugin has this method
        } else {
            printf("[RCBot Lua - No Engine/Plugin]: %s\n", message);
        }
    }
    return 0;
}

int Lua_RCBot_GetGameTime(lua_State* L) {
    float gameTime = 0.0f;
    if (g_pGlobals) {
        gameTime = g_pGlobals->curtime;
    } else if (g_pRCBotPlugin_Instance) { // Fallback to plugin's accessor if global not set directly
        // This assumes GetGlobals_Instance returns a valid CGlobalVarsBase*
        CGlobalVarsBase* plugin_globals = g_pRCBotPlugin_Instance->GetGlobals_Instance();
        if (plugin_globals) {
            gameTime = plugin_globals->curtime;
        } else {
            // static float placeholderTime = 0.0f; placeholderTime += 0.016f; gameTime = placeholderTime;
        }
    } else {
        // static float placeholderTime = 0.0f; placeholderTime += 0.016f; gameTime = placeholderTime;
    }
    lua_pushnumber(L, gameTime);
    return 1;
}

int Lua_RCBot_GetMapName_Conceptual(lua_State* L) {
    const char* mapNameStr = "unloaded_map_placeholder";
    if (g_pGlobals && g_pGlobals->mapname[0] != '\0' ) {
        mapNameStr = g_pGlobals->mapname;
    } else if (g_pRCBotPlugin_Instance) {
        CGlobalVarsBase* plugin_globals = g_pRCBotPlugin_Instance->GetGlobals_Instance();
        if (plugin_globals && plugin_globals->mapname[0] != '\0') {
            mapNameStr = plugin_globals->mapname;
        }
        // Fallback: CRCBotPlugin might store current map name itself
        // else if (g_pRCBotPlugin_Instance->GetCurrentMapName_SomeMethod()) { mapNameStr = g_pRCBotPlugin_Instance->GetCurrentMapName_SomeMethod(); }
    }
    lua_pushstring(L, mapNameStr);
    return 1;
}

} // namespace FFLuaBridge
