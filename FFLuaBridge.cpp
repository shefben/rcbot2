#include "FFLuaBridge.h"

// Assuming Lua headers are available in the include path.
// If not, they would need to be added to the project.
extern "C" {
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
}

#include <iostream> // For placeholder logging (Lua_RCBot_LogMessage)

namespace FFLuaBridge {

// --- Helper Function Implementations ---

std::string SafeGetStringField(lua_State* L, int tableIndex, const char* fieldName, const char* defaultValue) {
    lua_getfield(L, tableIndex, fieldName);
    const char* val = luaL_optstring(L, -1, defaultValue);
    lua_pop(L, 1); // Pop the value or nil
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
    }
    lua_pop(L, 1);
    return val;
}

bool LuaTableToVector(lua_State* L, int tableIndex, Vector& vec) {
    if (!lua_istable(L, tableIndex)) {
        // std::cerr << "FFLuaBridge Warning: Expected table for Vector, got " << lua_typename(L, lua_type(L, tableIndex)) << std::endl;
        return false; // Not a table
    }
    // Ensure tableIndex is absolute for safety if this function is called from various contexts
    int absTableIndex = lua_absindex(L, tableIndex);

    vec.x = (float)SafeGetNumberField(L, absTableIndex, "x", vec.x); // Default to existing value if field missing
    vec.y = (float)SafeGetNumberField(L, absTableIndex, "y", vec.y);
    vec.z = (float)SafeGetNumberField(L, absTableIndex, "z", vec.z);
    return true;
}

// --- Main Population Function Implementations ---

bool PopulateControlPointInfo(lua_State* L, const char* globalTableName, ControlPointInfo& cpInfo) {
    lua_getglobal(L, globalTableName);
    if (!lua_istable(L, -1)) {
        // std::cerr << "FFLuaBridge Warning: Global table '" << globalTableName << "' not found or not a table." << std::endl;
        lua_pop(L, 1); // Pop nil or non-table
        return false;
    }
    int tableIdx = lua_gettop(L); // Absolute index of the table

    cpInfo.id = SafeGetIntegerField(L, tableIdx, "id", -1);
    cpInfo.luaName = SafeGetStringField(L, tableIdx, "luaName", "");
    cpInfo.gameDisplayName = SafeGetStringField(L, tableIdx, "gameDisplayName", cpInfo.luaName.c_str());
    cpInfo.ownerTeam = SafeGetIntegerField(L, tableIdx, "ownerTeam", 0);
    cpInfo.captureProgress = (float)SafeGetNumberField(L, tableIdx, "captureProgress", 0.0);
    cpInfo.isLocked = SafeGetBooleanField(L, tableIdx, "isLocked", false);

    lua_getfield(L, tableIdx, "position");
    if (lua_istable(L, -1)) {
        LuaTableToVector(L, -1, cpInfo.position);
    } else {
        // std::cerr << "FFLuaBridge Warning: 'position' field missing or not a table in '" << globalTableName << "'." << std::endl;
    }
    lua_pop(L, 1); // Pop position table or nil

    lua_pop(L, 1); // Pop the global table
    return true;
}

bool PopulatePayloadWaypoint(lua_State* L, int tableIndex, PayloadWaypoint& waypoint) {
    if (!lua_istable(L, tableIndex)) return false;
    int absTableIndex = lua_absindex(L, tableIndex);

    lua_getfield(L, absTableIndex, "position");
    if (lua_istable(L, -1)) {
        LuaTableToVector(L, -1, waypoint.position);
    }
    lua_pop(L, 1);

    waypoint.desiredSpeedAtPoint = (float)SafeGetNumberField(L, absTableIndex, "desiredSpeedAtPoint", waypoint.desiredSpeedAtPoint);
    waypoint.isCheckPoint = SafeGetBooleanField(L, absTableIndex, "isCheckPoint", false);
    waypoint.eventOnReachLuaFunc = SafeGetStringField(L, absTableIndex, "eventOnReachLuaFunc", "");

    return true;
}

bool PopulatePayloadPathInfo(lua_State* L, const char* globalTableName, PayloadPathInfo& pathInfo) {
    lua_getglobal(L, globalTableName);
    if (!lua_istable(L, -1)) {
        // std::cerr << "FFLuaBridge Warning: Global table '" << globalTableName << "' not found or not a table." << std::endl;
        lua_pop(L, 1);
        return false;
    }
    int tableIdx = lua_gettop(L);

    pathInfo.pathId = SafeGetIntegerField(L, tableIdx, "pathId", 0);
    pathInfo.pathName = SafeGetStringField(L, tableIdx, "pathName", "");
    pathInfo.defaultSpeed = (float)SafeGetNumberField(L, tableIdx, "defaultSpeed", 75.0f);

    lua_getfield(L, tableIdx, "waypoints");
    if (lua_istable(L, -1)) {
        int waypointsTableIdx = lua_gettop(L);
        pathInfo.numWaypoints = 0;
        lua_pushnil(L); // First key for lua_next
        while (lua_next(L, waypointsTableIdx) != 0 && pathInfo.numWaypoints < MAX_PAYLOAD_WAYPOINTS_FF) {
            // key is at -2, value (waypoint table) is at -1
            if (lua_istable(L, -1)) {
                PopulatePayloadWaypoint(L, -1, pathInfo.waypoints[pathInfo.numWaypoints]);
                pathInfo.numWaypoints++;
            } else {
                // std::cerr << "FFLuaBridge Warning: Waypoint entry in '" << globalTableName << "' is not a table." << std::endl;
            }
            lua_pop(L, 1); // Pop value (waypoint table), keep key for next iteration
        }
    } else {
        // std::cerr << "FFLuaBridge Warning: 'waypoints' field missing or not a table in '" << globalTableName << "'." << std::endl;
    }
    lua_pop(L, 1); // Pop waypoints table or nil

    lua_pop(L, 1); // Pop the global table
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
    if (!lua_istable(L, -1)) {
        // std::cerr << "FFLuaBridge Warning: Global table '" << globalTableName << "' not found or not a table." << std::endl;
        lua_pop(L, 1);
        return false;
    }
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

    lua_pop(L, 1); // Pop the global table
    return true;
}


// --- C++ Functions to be Exposed to Lua ---

int Lua_RCBot_LogMessage(lua_State* L) {
    int n = lua_gettop(L); // Number of arguments
    if (n < 1) {
        return luaL_error(L, "RCBot.LogMessage expects at least 1 argument (message string).");
    }
    const char* message = luaL_checkstring(L, 1);
    // In a real plugin, this would call into CRCBotPlugin's logging system
    // e.g., if (g_pRCBotPlugin) g_pRCBotPlugin->LogMessageToConsole(message);
    printf("[RCBot Lua]: %s\n", message); // Using printf for basic output
    // std::cout << "[RCBot Lua]: " << message << std::endl;
    return 0; // Number of return values
}

int Lua_RCBot_GetGameTime(lua_State* L) {
    // In a real plugin, this would get time from the engine
    // e.g., if (g_pRCBotPlugin) lua_pushnumber(L, g_pRCBotPlugin->GetCurrentGameTime());
    // else lua_pushnumber(L, 0.0);
    lua_pushnumber(L, 123.45); // Placeholder time
    return 1; // One return value
}

// Example implementation for Lua_RCBot_GetControlPointStatus (conceptual)
// int Lua_RCBot_GetControlPointStatus(lua_State* L) {
//     const char* cpLuaName = luaL_checkstring(L, 1);
//     if (!cpLuaName) return luaL_error(L, "RCBot.GetControlPointStatus expects a CP Lua name string.");
//
//     // Conceptual: Access to game state, e.g., through CRCBotPlugin or a global knowledge base
//     // if (g_pRCBotPlugin && g_pRCBotPlugin->GetKnowledgeBase()) {
//     //     const ControlPointInfo* cpInfo = g_pRCBotPlugin->GetKnowledgeBase()->GetControlPointByLuaName(cpLuaName);
//     //     if (cpInfo) {
//     //         lua_newtable(L); // Create result table
//     //         lua_pushinteger(L, cpInfo->id); lua_setfield(L, -2, "id");
//     //         lua_pushstring(L, cpInfo->gameDisplayName.c_str()); lua_setfield(L, -2, "name");
//     //         lua_pushinteger(L, cpInfo->ownerTeam); lua_setfield(L, -2, "ownerTeam");
//     //         lua_pushnumber(L, cpInfo->captureProgress); lua_setfield(L, -2, "progress");
//     //         lua_pushboolean(L, cpInfo->isLocked); lua_setfield(L, -2, "isLocked");
//     //         lua_newtable(L); // Position table
//     //             lua_pushnumber(L, cpInfo->position.x); lua_setfield(L, -2, "x");
//     //             lua_pushnumber(L, cpInfo->position.y); lua_setfield(L, -2, "y");
//     //             lua_pushnumber(L, cpInfo->position.z); lua_setfield(L, -2, "z");
//     //         lua_setfield(L, -2, "position"); // Set position table into result table
//     //         return 1; // Return the result table
//     //     }
//     // }
//     lua_pushnil(L); // CP not found or system not ready
//     return 1;
// }


} // namespace FFLuaBridge
