#ifndef LUABRIDGE_H
#define LUABRIDGE_H

// Forward declare lua_State from Lua headers
struct lua_State;

namespace LuaBridge {

    bool InitializeLua();
    void ShutdownLua();
    void RegisterLuaFunctions(); // To register C++ functions with Lua
    bool ExecuteLuaScript(const char* scriptPath);

    // Functions to be called from Lua
    int Cpp_PrintMessageToConsole(lua_State* L);
    int Cpp_GetControlPointData(lua_State* L); // Placeholder

} // namespace LuaBridge

#endif // LUABRIDGE_H
