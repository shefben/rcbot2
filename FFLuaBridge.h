#ifndef FF_LUA_BRIDGE_H
#define FF_LUA_BRIDGE_H

#include <string>
#include <vector>
#include "FFStateStructs.h"
struct lua_State;

namespace FFLuaBridge {

    // --- Helper Functions ---
    std::string SafeGetStringField(lua_State* L, int tableIndex, const char* fieldName, const char* defaultValue = "");
    int SafeGetIntegerField(lua_State* L, int tableIndex, const char* fieldName, int defaultValue = 0);
    double SafeGetNumberField(lua_State* L, int tableIndex, const char* fieldName, double defaultValue = 0.0);
    bool SafeGetBooleanField(lua_State* L, int tableIndex, const char* fieldName, bool defaultValue = false);
    bool LuaTableToVector(lua_State* L, int tableIndex, Vector& vec);

    // --- Main Population Functions ---
    bool PopulateControlPointInfo(lua_State* L, const char* globalTableName, ControlPointInfo& cpInfo);
    bool PopulatePayloadPathInfo(lua_State* L, const char* globalTableName, PayloadPathInfo& pathInfo);
    bool PopulatePayloadWaypoint(lua_State* L, int tableIndex, PayloadWaypoint& waypoint);
    bool PopulateClassConfigInfo(lua_State* L, const char* globalTableName, ClassConfigInfo& classInfo);
    bool PopulateClassWeaponInfo(lua_State* L, int tableIndex, ClassWeaponInfo& weaponInfo);

    // --- C++ Functions to be Exposed to Lua ---
    int Lua_RCBot_LogMessage(lua_State* L);
    int Lua_RCBot_GetGameTime(lua_State* L);
    int Lua_RCBot_GetMapName_Conceptual(lua_State* L); // Added

    // int Lua_RCBot_GetControlPointStatus(lua_State* L); // Example from before

} // namespace FFLuaBridge

#endif // FF_LUA_BRIDGE_H
