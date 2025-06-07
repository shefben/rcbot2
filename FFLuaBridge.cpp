#include "FFLuaBridge.h"
#include "CRCBotPlugin.h" // For g_pRCBotPlugin_Instance (if used) or engine/globals accessors

// Lua headers
extern "C" {
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
}

#include <iostream> // For placeholder logging if engine console not available

// --- Conceptual Globals (would be properly managed/accessed in CRCBotPlugin) ---
// extern IVEngineServer* g_pEngineServer; // Assume CRCBotPlugin provides access if needed
// extern CGlobalVarsBase* gpGlobals;     // Assume CRCBotPlugin provides access if needed
// For FFLuaBridge functions to access plugin members like engine interfaces or game time,
// they typically need a pointer to the CRCBotPlugin instance.
// Using g_pRCBotPlugin_Instance which is set in CRCBotPlugin.cpp.
extern CRCBotPlugin* g_pRCBotPlugin_Instance;
// --- End Conceptual Globals ---


namespace FFLuaBridge {

// --- Helper Function Implementations (from previous step, ensure they are robust) ---

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
    if (lua_isboolean(L, -1)) {
        val = lua_toboolean(L, -1);
    } else if (!lua_isnil(L, -1)) { // Treat non-nil, non-boolean as true if not strictly typed
        // val = true; // Or log a warning: luaL_argerror(L, tableIndex, "boolean expected");
    }
    lua_pop(L, 1);
    return val;
}

bool LuaTableToVector(lua_State* L, int tableIndex, Vector& vec) {
    if (!lua_istable(L, tableIndex)) {
        return false;
    }
    int absTableIndex = lua_absindex(L, tableIndex);
    vec.x = (float)SafeGetNumberField(L, absTableIndex, "x", vec.x);
    vec.y = (float)SafeGetNumberField(L, absTableIndex, "y", vec.y);
    vec.z = (float)SafeGetNumberField(L, absTableIndex, "z", vec.z);
    return true;
}

// --- Main Population Function Implementations (from previous step) ---
bool PopulateControlPointInfo(lua_State* L, const char* globalTableName, ControlPointInfo& cpInfo) {
    lua_getglobal(L, globalTableName);
    if (!lua_istable(L, -1)) {
        lua_pop(L, 1);
        return false;
    }
    int tableIdx = lua_gettop(L);

    cpInfo.id = SafeGetIntegerField(L, tableIdx, "id", -1);
    cpInfo.luaName = SafeGetStringField(L, tableIdx, "luaName", "");
    cpInfo.gameDisplayName = SafeGetStringField(L, tableIdx, "gameDisplayName", cpInfo.luaName.c_str());
    cpInfo.ownerTeam = SafeGetIntegerField(L, tableIdx, "ownerTeam", 0);
    cpInfo.captureProgress = (float)SafeGetNumberField(L, tableIdx, "captureProgress", 0.0);
    cpInfo.isLocked = SafeGetBooleanField(L, tableIdx, "isLocked", false);

    lua_getfield(L, tableIdx, "position");
    if (lua_istable(L, -1)) {
        LuaTableToVector(L, -1, cpInfo.position);
    }
    lua_pop(L, 1);

    lua_pop(L, 1);
    return true;
}

bool PopulatePayloadWaypoint(lua_State* L, int tableIndex, PayloadWaypoint& waypoint) {
    if (!lua_istable(L, tableIndex)) return false;
    int absTableIndex = lua_absindex(L, tableIndex);

    lua_getfield(L, absTableIndex, "position");
    if (lua_istable(L, -1)) LuaTableToVector(L, -1, waypoint.position);
    lua_pop(L, 1);

    waypoint.desiredSpeedAtPoint = (float)SafeGetNumberField(L, absTableIndex, "desiredSpeedAtPoint", waypoint.desiredSpeedAtPoint);
    waypoint.isCheckPoint = SafeGetBooleanField(L, absTableIndex, "isCheckPoint", false);
    waypoint.eventOnReachLuaFunc = SafeGetStringField(L, absTableIndex, "eventOnReachLuaFunc", "");
    return true;
}

bool PopulatePayloadPathInfo(lua_State* L, const char* globalTableName, PayloadPathInfo& pathInfo) {
    lua_getglobal(L, globalTableName);
    if (!lua_istable(L, -1)) { lua_pop(L, 1); return false; }
    int tableIdx = lua_gettop(L);

    pathInfo.pathId = SafeGetIntegerField(L, tableIdx, "pathId", 0);
    pathInfo.pathName = SafeGetStringField(L, tableIdx, "pathName", "");
    pathInfo.defaultSpeed = (float)SafeGetNumberField(L, tableIdx, "defaultSpeed", 75.0f);

    lua_getfield(L, tableIdx, "waypoints");
    if (lua_istable(L, -1)) {
        int waypointsTableIdx = lua_gettop(L);
        pathInfo.numWaypoints = 0;
        lua_pushnil(L);
        while (lua_next(L, waypointsTableIdx) != 0 && pathInfo.numWaypoints < MAX_PAYLOAD_WAYPOINTS_FF) {
            if (lua_istable(L, -1)) { // Value is the waypoint table
                PopulatePayloadWaypoint(L, -1, pathInfo.waypoints[pathInfo.numWaypoints]);
                pathInfo.numWaypoints++;
            }
            lua_pop(L, 1);
        }
    }
    lua_pop(L, 1);
    lua_pop(L, 1);
    return true;
}

bool PopulateClassWeaponInfo(lua_State* L, int tableIndex, ClassWeaponInfo& weaponInfo) {
    if (!lua_istable(L, tableIndex)) return false;
    int absTableIndex = lua_absindex(L, tableIndex);
    weaponInfo.weaponNameId = SafeGetStringField(L, absTableIndex, "weaponNameId", "");
    weaponInfo.slot = SafeGetIntegerField(L, absTableIndex, "slot", -1);
    weaponInfo.maxAmmoClip = SafeGetIntegerField(L, absTableIndex, "maxAmmoClip", 0);
    weaponInfo.maxAmmoReserve = SafeGetIntegerField(L, absTableIndex, "maxAmmoReserve", 0);
    return true;
}

bool PopulateClassConfigInfo(lua_State* L, const char* globalTableName, ClassConfigInfo& classInfo) {
    lua_getglobal(L, globalTableName);
    if (!lua_istable(L, -1)) { lua_pop(L, 1); return false; }
    int tableIdx = lua_gettop(L);

    classInfo.classId = SafeGetIntegerField(L, tableIdx, "classId", -1);
    classInfo.className = SafeGetStringField(L, tableIdx, "className", "");
    classInfo.luaBotClassName = SafeGetStringField(L, tableIdx, "luaBotClassName", classInfo.className.c_str());
    classInfo.maxHealth = SafeGetIntegerField(L, tableIdx, "maxHealth", 100);
    classInfo.maxArmor = SafeGetIntegerField(L, tableIdx, "maxArmor", 0);
    classInfo.armorType = SafeGetStringField(L, tableIdx, "armorType", "");
    classInfo.speed = (float)SafeGetNumberField(L, tableIdx, "speed", 300.0);

    lua_getfield(L, tableIdx, "mins");
    if (lua_istable(L, -1)) LuaTableToVector(L, -1, classInfo.mins);
    lua_pop(L, 1);
    lua_getfield(L, tableIdx, "maxs");
    if (lua_istable(L, -1)) LuaTableToVector(L, -1, classInfo.maxs);
    lua_pop(L, 1);

    lua_getfield(L, tableIdx, "availableWeapons");
    if (lua_istable(L, -1)) {
        int weaponsTableIdx = lua_gettop(L);
        classInfo.availableWeapons.clear();
        lua_pushnil(L);
        while (lua_next(L, weaponsTableIdx) != 0) {
            if (lua_istable(L, -1)) {
                ClassWeaponInfo weapon;
                PopulateClassWeaponInfo(L, -1, weapon);
                classInfo.availableWeapons.push_back(weapon);
            }
            lua_pop(L, 1);
        }
    }
    lua_pop(L, 1);
    lua_pop(L, 1);
    return true;
}


// --- C++ Functions to be Exposed to Lua ---

int Lua_RCBot_LogMessage(lua_State* L) {
    const char* message = luaL_checkstring(L, 1); // Check and get first argument
    if (g_pRCBotPlugin_Instance) {
        // Conceptual: g_pRCBotPlugin_Instance->GetEngineServer()->ConMsg("[RCBot Lua]: %s\n", message);
        // Or: g_pRCBotPlugin_Instance->LogPluginMessage("[RCBot Lua]: %s", message);
        // For now, using std::cout via a plugin method if it exists or directly.
        // Assuming CRCBotPlugin has a LogMessageToConsole for this purpose.
        // g_pRCBotPlugin_Instance->LogMessageToConsole(message); // If CRCBotPlugin has such method
        printf("[RCBot Lua]: %s\n", message); // Fallback to printf
    } else {
        printf("[RCBot Lua (No Plugin Instance)]: %s\n", message);
    }
    return 0; // Number of return values
}

int Lua_RCBot_GetGameTime(lua_State* L) {
    float gameTime = 0.0f;
    if (g_pRCBotPlugin_Instance) {
        // gameTime = g_pRCBotPlugin_Instance->GetGlobals()->curtime; // Conceptual
        // For now, use the plugin's GetCurrentGameTime which might have its own placeholder
        gameTime = g_pRCBotPlugin_Instance->GetCurrentGameTime();
    } else {
        // Fallback if no plugin instance (e.g. during early test of Lua only)
        // static float dummyTime = 0.0f; dummyTime += 0.1f; gameTime = dummyTime;
    }
    lua_pushnumber(L, gameTime);
    return 1; // One return value
}

// int Lua_RCBot_GetMapName(lua_State* L) { // Example if added
//     const char* mapName = "";
//     if (g_pRCBotPlugin_Instance) {
//         mapName = g_pRCBotPlugin_Instance->GetCurrentMapName();
//     }
//     lua_pushstring(L, mapName);
//     return 1;
// }


} // namespace FFLuaBridge
