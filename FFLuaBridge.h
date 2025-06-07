#ifndef FF_LUA_BRIDGE_H
#define FF_LUA_BRIDGE_H

#include <string>
#include <vector>
#include "FFStateStructs.h" // Contains Vector, ControlPointInfo, etc.

// Forward declare lua_State
struct lua_State;

namespace FFLuaBridge {

    // --- Helper Functions ---

    // Helper to safely get a string field from a table at the given absolute stack index
    std::string SafeGetStringField(lua_State* L, int tableIndex, const char* fieldName, const char* defaultValue = "");
    // Helper to safely get an integer field
    int SafeGetIntegerField(lua_State* L, int tableIndex, const char* fieldName, int defaultValue = 0);
    // Helper to safely get a number (float/double) field
    double SafeGetNumberField(lua_State* L, int tableIndex, const char* fieldName, double defaultValue = 0.0);
    // Helper to safely get a boolean field
    bool SafeGetBooleanField(lua_State* L, int tableIndex, const char* fieldName, bool defaultValue = false);

    // Helper to populate a Vector struct from a Lua table at the given absolute stack index
    bool LuaTableToVector(lua_State* L, int tableIndex, Vector& vec);

    // --- Main Population Functions ---
    // These functions expect the global Lua table name and will handle getting it onto the stack.

    // Populates ControlPointInfo from a global Lua table.
    bool PopulateControlPointInfo(lua_State* L, const char* globalTableName, ControlPointInfo& cpInfo);

    // Populates PayloadPathInfo from a global Lua table.
    bool PopulatePayloadPathInfo(lua_State* L, const char* globalTableName, PayloadPathInfo& pathInfo);

    // Populates a single PayloadWaypoint from a Lua table at the given absolute stack index.
    bool PopulatePayloadWaypoint(lua_State* L, int tableIndex, PayloadWaypoint& waypoint);

    // Populates ClassConfigInfo from a global Lua table.
    bool PopulateClassConfigInfo(lua_State* L, const char* globalTableName, ClassConfigInfo& classInfo);

    // Populates a single ClassWeaponInfo from a Lua table at the given absolute stack index.
    bool PopulateClassWeaponInfo(lua_State* L, int tableIndex, ClassWeaponInfo& weaponInfo);


    // --- C++ Functions to be Exposed to Lua ---
    // These are the actual C functions that Lua will call.

    // Logs a message from Lua to the server console (placeholder).
    // Lua usage: RCBot.LogMessage("Hello from Lua!")
    int Lua_RCBot_LogMessage(lua_State* L);

    // Gets the current game time (placeholder).
    // Lua usage: local time = RCBot.GetGameTime()
    int Lua_RCBot_GetGameTime(lua_State* L);

    // Example: Function to get a control point's status
    // Lua usage: local cp_status = RCBot.GetControlPointStatus("cp_middle")
    // Returns a table: { id=1, ownerTeam=2, progress=0.5, isLocked=false, position={x=1,y=2,z=3} }
    // int Lua_RCBot_GetControlPointStatus(lua_State* L);


} // namespace FFLuaBridge

#endif // FF_LUA_BRIDGE_H
