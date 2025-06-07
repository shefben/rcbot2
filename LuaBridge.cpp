#include "LuaBridge.h"
#include "plugin_define.h" // For engine->Con_Printf
#include "lua/lua.hpp"     // Lua headers

#include <string> // For std::string if used

// Global Lua state
lua_State* g_pLuaState = nullptr;

// Conceptual engine pointer, needs to be set in plugin_main.cpp
IVEngineServer* engine = nullptr;
// ICvar* cvar = nullptr; // Also needs to be set if LuaBridge uses it directly

namespace LuaBridge {

    bool InitializeLua() {
        if (g_pLuaState) {
            if (engine) engine->Con_Printf("LuaBridge: Lua state already initialized.\n");
            return true; // Already initialized
        }

        g_pLuaState = luaL_newstate();
        if (!g_pLuaState) {
            if (engine) engine->Con_Printf("LuaBridge: Failed to create Lua state.\n");
            return false;
        }

        luaL_openlibs(g_pLuaState);
        if (engine) engine->Con_Printf("LuaBridge: Lua state created and libraries opened.\n");

        RegisterLuaFunctions(); // Register C++ functions
        return true;
    }

    void ShutdownLua() {
        if (g_pLuaState) {
            lua_close(g_pLuaState);
            g_pLuaState = nullptr;
            if (engine) engine->Con_Printf("LuaBridge: Lua state closed.\n");
        } else {
            if (engine) engine->Con_Printf("LuaBridge: No active Lua state to close.\n");
        }
    }

    void RegisterLuaFunctions() {
        if (!g_pLuaState) {
            if (engine) engine->Con_Printf("LuaBridge: Cannot register functions, Lua state not initialized.\n");
            return;
        }
        if (engine) engine->Con_Printf("LuaBridge: Registering C++ functions with Lua...\n");

        lua_register(g_pLuaState, "PrintMessageToConsole", Cpp_PrintMessageToConsole);
        lua_register(g_pLuaState, "GetControlPointData", Cpp_GetControlPointData);

        if (engine) engine->Con_Printf("LuaBridge: Functions registered.\n");
    }

    bool ExecuteLuaScript(const char* scriptPath) {
        if (!g_pLuaState) {
            if (engine) engine->Con_Printf("LuaBridge: Cannot execute script, Lua state not initialized.\n");
            return false;
        }
        if (engine) engine->Con_Printf("LuaBridge: Attempting to execute Lua script: %s\n", scriptPath);

        if (luaL_dofile(g_pLuaState, scriptPath) != LUA_OK) {
            const char* errorMsg = lua_tostring(g_pLuaState, -1);
            if (engine) engine->Con_Printf("LuaBridge: Error executing script %s: %s\n", scriptPath, errorMsg);
            lua_pop(g_pLuaState, 1); // Pop error message from stack
            return false;
        }

        if (engine) engine->Con_Printf("LuaBridge: Lua script %s executed successfully.\n", scriptPath);
        return true;
    }

    // Functions to be called from Lua
    int Cpp_PrintMessageToConsole(lua_State* L) {
        int n_args = lua_gettop(L);
        if (n_args != 1) {
            if (engine) engine->Con_Printf("Cpp_PrintMessageToConsole: Error! Expected 1 argument, got %d\n", n_args);
            return 0; // Number of return values
        }
        const char* msg = luaL_checkstring(L, 1);
        if (engine) {
            // Using ServerCommand to echo, as Con_Printf might not be ideal for messages from Lua directly
            // or if we want it to appear like a server message.
            std::string cmd = "echo \"Lua: ";
            cmd += msg;
            cmd += "\"";
            engine->ServerCommand(cmd.c_str());
            engine->Con_Printf("Cpp_PrintMessageToConsole: Relayed message from Lua: %s\n", msg);
        } else {
            // Fallback if engine pointer isn't available (should not happen in normal operation)
            printf("Cpp_PrintMessageToConsole (no engine): %s\n", msg);
        }
        return 0; // Number of return values
    }

    int Cpp_GetControlPointData(lua_State* L) {
        if (engine) engine->Con_Printf("Cpp_GetControlPointData: Called from Lua. Returning dummy data.\n");

        lua_newtable(L); // Create main table to return

        // CP 1 (Example dummy data)
        lua_pushinteger(L, 1);   // Key for CP ID (e.g., 1)
        lua_newtable(L);         // Value: table for CP1 data

        lua_pushstring(L, "cp_example_mid"); // name field
        lua_setfield(L, -2, "name");

        lua_pushinteger(L, 0); // owner field (0 = uncaptured, 1 = red, 2 = blu etc.)
        lua_setfield(L, -2, "owner");

        lua_pushnumber(L, 0.5); // progress field
        lua_setfield(L, -2, "progress");

        lua_settable(L, -3); // Add CP1 table to main table, using its ID as key

        // CP 2 (Example dummy data)
        lua_pushinteger(L, 2);   // Key for CP ID (e.g., 2)
        lua_newtable(L);         // Value: table for CP2 data

        lua_pushstring(L, "cp_example_last"); // name field
        lua_setfield(L, -2, "name");

        lua_pushinteger(L, 1); // owner field
        lua_setfield(L, -2, "owner");

        lua_pushnumber(L, 0.0); // progress field
        lua_setfield(L, -2, "progress");

        lua_settable(L, -3); // Add CP2 table to main table

        return 1; // Number of return values (the main table)
    }

} // namespace LuaBridge
