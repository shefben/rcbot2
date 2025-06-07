#include "ff_state_structs.h"

// Assume Lua headers are available in the include path
// For a typical Lua installation, lua.hpp includes all necessary headers.
// If using plain C, include lua.h, lualib.h, lauxlib.h separately.
extern "C" {
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
}

#include <iostream> // For basic logging/warnings in these functions

// Helper function to populate a Vector from a Lua table on the stack
bool PopulateVector(lua_State* L, int tableIndex, Vector& vec) {
    if (!lua_istable(L, tableIndex)) {
        // Optionally log: tableIndex is not a table
        return false;
    }

    // Ensure tableIndex is absolute to avoid confusion with relative stack positions
    int absoluteTableIndex = lua_absindex(L, tableIndex);

    lua_getfield(L, absoluteTableIndex, "x");
    vec.x = (float)luaL_optnumber(L, -1, 0.0);
    lua_pop(L, 1);

    lua_getfield(L, absoluteTableIndex, "y");
    vec.y = (float)luaL_optnumber(L, -1, 0.0);
    lua_pop(L, 1);

    lua_getfield(L, absoluteTableIndex, "z");
    vec.z = (float)luaL_optnumber(L, -1, 0.0);
    lua_pop(L, 1);

    return true;
}

// Populate ControlPointInfo from a global Lua table
bool PopulateControlPointInfo(lua_State* L, ControlPointInfo& cpInfo, const char* cpTableName) {
    lua_getglobal(L, cpTableName);
    if (!lua_istable(L, -1)) {
        std::cerr << "Warning: Lua table '" << cpTableName << "' not found or not a table." << std::endl;
        lua_pop(L, 1); // Pop the nil or non-table value
        return false;
    }
    int tableIdx = lua_gettop(L);

    lua_getfield(L, tableIdx, "id");
    cpInfo.id = (int)luaL_optinteger(L, -1, -1);
    lua_pop(L, 1);

    lua_getfield(L, tableIdx, "luaName");
    cpInfo.luaName = luaL_optstring(L, -1, "");
    lua_pop(L, 1);

    lua_getfield(L, tableIdx, "teamOwner");
    cpInfo.teamOwner = (int)luaL_optinteger(L, -1, 0);
    lua_pop(L, 1);

    lua_getfield(L, tableIdx, "captureProgress");
    cpInfo.captureProgress = (float)luaL_optnumber(L, -1, 0.0);
    lua_pop(L, 1);

    lua_getfield(L, tableIdx, "isLocked");
    cpInfo.isLocked = lua_toboolean(L, -1);
    lua_pop(L, 1);

    lua_getfield(L, tableIdx, "position");
    if (lua_istable(L, -1)) {
        PopulateVector(L, -1, cpInfo.position); // -1 refers to the table at the top of the stack
    } else {
        std::cerr << "Warning: 'position' table not found or not a table in '" << cpTableName << "'." << std::endl;
    }
    lua_pop(L, 1); // Pop the position table or nil

    lua_pop(L, 1); // Pop the main cpTableName table
    return true;
}

// Populate PayloadPathInfo from a global Lua table
bool PopulatePayloadPathInfo(lua_State* L, PayloadPathInfo& pathInfo, const char* pathTableName) {
    lua_getglobal(L, pathTableName);
    if (!lua_istable(L, -1)) {
        std::cerr << "Warning: Lua table '" << pathTableName << "' not found or not a table." << std::endl;
        lua_pop(L, 1);
        return false;
    }
    int tableIdx = lua_gettop(L);

    lua_getfield(L, tableIdx, "pathId");
    pathInfo.pathId = (int)luaL_optinteger(L, -1, 0);
    lua_pop(L, 1);

    lua_getfield(L, tableIdx, "pathName");
    pathInfo.pathName = luaL_optstring(L, -1, "");
    lua_pop(L, 1);

    lua_getfield(L, tableIdx, "defaultSpeed");
    pathInfo.defaultSpeed = (float)luaL_optnumber(L, -1, 100.0);
    lua_pop(L, 1);

    lua_getfield(L, tableIdx, "waypoints");
    if (lua_istable(L, -1)) {
        int waypointsTableIdx = lua_gettop(L);
        pathInfo.numWaypoints = 0;
        lua_pushnil(L); // First key for lua_next
        while (lua_next(L, waypointsTableIdx) != 0 && pathInfo.numWaypoints < MAX_PAYLOAD_WAYPOINTS) {
            // key is at -2, value (waypoint table) is at -1
            if (lua_istable(L, -1)) {
                int waypointTableStackIdx = lua_gettop(L);
                PayloadWaypoint& wp = pathInfo.waypoints[pathInfo.numWaypoints];

                lua_getfield(L, waypointTableStackIdx, "position");
                if (lua_istable(L, -1)) {
                    PopulateVector(L, -1, wp.position);
                } else {
                     std::cerr << "Warning: 'position' table not found in a waypoint for '" << pathTableName << "'." << std::endl;
                }
                lua_pop(L, 1); // Pop position table or nil

                lua_getfield(L, waypointTableStackIdx, "desiredSpeedAtPoint");
                wp.desiredSpeedAtPoint = (float)luaL_optnumber(L, -1, pathInfo.defaultSpeed);
                lua_pop(L, 1);

                pathInfo.numWaypoints++;
            } else {
                std::cerr << "Warning: Waypoint entry is not a table in '" << pathTableName << "'." << std::endl;
            }
            lua_pop(L, 1); // Pop value (waypoint table), keep key for next iteration
        }
    } else {
        std::cerr << "Warning: 'waypoints' table not found or not a table in '" << pathTableName << "'." << std::endl;
    }
    lua_pop(L, 1); // Pop waypoints table or nil

    lua_pop(L, 1); // Pop main pathTableName table
    return true;
}

// Populate ClassConfigInfo from a global Lua table
bool PopulateClassConfigInfo(lua_State* L, ClassConfigInfo& classInfo, const char* classTableName) {
    lua_getglobal(L, classTableName);
    if (!lua_istable(L, -1)) {
        std::cerr << "Warning: Lua table '" << classTableName << "' not found or not a table." << std::endl;
        lua_pop(L, 1);
        return false;
    }
    int tableIdx = lua_gettop(L);

    lua_getfield(L, tableIdx, "classId");
    classInfo.classId = (int)luaL_optinteger(L, -1, -1);
    lua_pop(L, 1);

    lua_getfield(L, tableIdx, "className");
    classInfo.className = luaL_optstring(L, -1, "");
    lua_pop(L, 1);

    lua_getfield(L, tableIdx, "luaBotClassName");
    classInfo.luaBotClassName = luaL_optstring(L, -1, "");
    lua_pop(L, 1);

    lua_getfield(L, tableIdx, "maxHealth");
    classInfo.maxHealth = (int)luaL_optinteger(L, -1, 0);
    lua_pop(L, 1);

    lua_getfield(L, tableIdx, "speed");
    classInfo.speed = (float)luaL_optnumber(L, -1, 0.0);
    lua_pop(L, 1);

    lua_getfield(L, tableIdx, "availableWeapons");
    if (lua_istable(L, -1)) {
        int weaponsTableIdx = lua_gettop(L);
        classInfo.availableWeapons.clear();
        lua_pushnil(L); // First key for lua_next
        while (lua_next(L, weaponsTableIdx) != 0) {
            // key is at -2, value (weapon table) is at -1
            if (lua_istable(L, -1)) {
                int weaponTableStackIdx = lua_gettop(L);
                ClassWeaponInfo weapon;

                lua_getfield(L, weaponTableStackIdx, "weaponName");
                weapon.weaponName = luaL_optstring(L, -1, "");
                lua_pop(L, 1);

                lua_getfield(L, weaponTableStackIdx, "slot");
                weapon.slot = (int)luaL_optinteger(L, -1, -1);
                lua_pop(L, 1);

                classInfo.availableWeapons.push_back(weapon);
            } else {
                 std::cerr << "Warning: Weapon entry is not a table in '" << classTableName << "'." << std::endl;
            }
            lua_pop(L, 1); // Pop value (weapon table), keep key for next iteration
        }
    } else {
        std::cerr << "Warning: 'availableWeapons' table not found or not a table in '" << classTableName << "'." << std::endl;
    }
    lua_pop(L, 1); // Pop availableWeapons table or nil

    lua_pop(L, 1); // Pop main classTableName table
    return true;
}

/*
// --- Conceptual Example Usage (to be part of CRCBotPlugin or similar) ---

// CRCBotPlugin* g_pRCBotPlugin = nullptr; // Assuming a global or accessible plugin instance

// void LoadMapDataFromLua(lua_State* L, const char* mapName) {
//     if (!L || !g_pRCBotPlugin) return;

//     std::string scriptPath = "maps/";
//     scriptPath += mapName;
//     scriptPath += "_data.lua"; // e.g., "maps/ff_2fort_data.lua"

//     // g_pRCBotPlugin->ExecuteLuaScript(scriptPath.c_str()); // Conceptual

//     // Example for one control point
//     ControlPointInfo cp1_info;
//     if (PopulateControlPointInfo(L, cp1_info, "g_ControlPoint_CP1")) {
//         // Store cp1_info, e.g., g_pRCBotPlugin->m_ControlPoints.push_back(cp1_info);
//         std::cout << "Loaded CP1: " << cp1_info.luaName << " at "
//                   << cp1_info.position.x << "," << cp1_info.position.y << "," << cp1_info.position.z
//                   << std::endl;
//     }

//     // Example for one payload path
//     PayloadPathInfo payload_path_main;
//     if (PopulatePayloadPathInfo(L, payload_path_main, "g_PayloadPath_Main")) {
//         // Store payload_path_main
//         std::cout << "Loaded Payload Path: " << payload_path_main.pathName
//                   << " with " << payload_path_main.numWaypoints << " waypoints." << std::endl;
//     }

//     // Example for one class config
//     ClassConfigInfo scout_config;
//     if (PopulateClassConfigInfo(L, scout_config, "g_ClassConfig_Scout")) {
//         // Store scout_config
//         std::cout << "Loaded Class Config: " << scout_config.className
//                   << " with max health " << scout_config.maxHealth << std::endl;
//         for (const auto& weapon : scout_config.availableWeapons) {
//             std::cout << "  Weapon: " << weapon.weaponName << " in slot " << weapon.slot << std::endl;
//         }
//     }
// }
*/

```

**Key considerations for this implementation:**

*   **Lua Headers:** The `extern "C"` block is standard practice for including C Lua headers in a C++ file.
*   **Error Handling & Logging:** Basic error messages are printed to `std::cerr`. In a real plugin, this would use the engine's logging mechanism (e.g., `ConMsg` or `Warning` from `IVEngineServer` or `ICvar`).
*   **`luaL_optstring`, `luaL_optnumber`, `luaL_optinteger`:** These are useful for safely getting values from Lua tables, providing default values if a field is missing or of the wrong type.
*   **Stack Management:** `lua_pop(L, 1)` is used to remove values from the Lua stack after they are read, keeping the stack clean. `lua_absindex` is used in `PopulateVector` to make sure the table index is absolute, which is safer when passing stack indices to helper functions.
*   **Table Iteration:** `lua_next` is used to iterate over tables (e.g., for `waypoints` and `availableWeapons`).
*   **Simplicity:** For this subtask, the functions are standalone. In the full plugin, they'd likely be static members of `CRCBotPlugin` or a dedicated `LuaBridge` helper class, and logging would go through the engine.
*   **`MAX_PAYLOAD_WAYPOINTS`:** Using a fixed-size array for `waypoints` is simpler for this example. A `std::vector` could be used but would involve more dynamic memory management during Lua table parsing.

The conceptual example usage is commented out at the end but illustrates how these functions would be called after a Lua script defining the data tables has been executed.
