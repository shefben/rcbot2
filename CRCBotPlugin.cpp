#include "CRCBotPlugin.h"
#include "ff_lua_bridge.h" // Contains Populate... functions

// Lua headers
extern "C" {
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
}

#include <iostream> // For std::cout in this example, replace with engine logging

// Define the global plugin pointer
CRCBotPlugin* g_pRCBotPlugin = nullptr;

// Assume engine global or an interface to it exists for ConMsg
// For example, if IVEngineServer is obtained and stored in m_pEngineServer:
// #define Msg (g_pRCBotPlugin->m_pEngineServer->ConMsg)
// For now, we'll use std::cout for LogMessageToConsole for simplicity here.

// --- Constructor / Destructor ---
CRCBotPlugin::CRCBotPlugin() :
    m_pLuaState(nullptr),
    m_bLuaBridgeInitialized(false),
    m_pEngineServer(nullptr), // Initialize interface pointers
    m_CurrentMapName("")
{
    g_pRCBotPlugin = this; // Set the global pointer
}

CRCBotPlugin::~CRCBotPlugin() {
    ShutdownLuaBridge();
    g_pRCBotPlugin = nullptr;
}

// --- IServerPluginCallbacks (Conceptual Implementations) ---
bool CRCBotPlugin::Load(CreateInterfaceFn interfaceFactory, CreateInterfaceFn gameServerFactory) {
    // Conceptual: Get IVEngineServer interface
    // if (interfaceFactory) {
    //     m_pEngineServer = (IVEngineServer*)interfaceFactory(INTERFACEVERSION_VENGINESERVER, nullptr);
    //     if (!m_pEngineServer) {
    //         std::cerr << "Error: Could not get IVEngineServer interface!" << std::endl;
    //         return false;
    //     }
    // }
    std::cout << "CRCBotPlugin::Load - Plugin Loaded (Conceptual)" << std::endl;
    // Note: Lua bridge is typically initialized on LevelInit or ServerActivate,
    // once the game and map data are more likely to be available.
    return true;
}

void CRCBotPlugin::Unload() {
    std::cout << "CRCBotPlugin::Unload - Plugin Unloaded (Conceptual)" << std::endl;
}

void CRCBotPlugin::Pause() {
    std::cout << "CRCBotPlugin::Pause (Conceptual)" << std::endl;
}

void CRCBotPlugin::UnPause() {
    std::cout << "CRCBotPlugin::UnPause (Conceptual)" << std::endl;
}

const char* CRCBotPlugin::GetPluginDescription() {
    return "RCBot Plugin v0.1";
}

void CRCBotPlugin::LevelInit(char const *pMapName) {
    m_CurrentMapName = pMapName ? pMapName : "";
    std::cout << "CRCBotPlugin::LevelInit - Map: " << m_CurrentMapName << " (Conceptual)" << std::endl;

    // Initialize Lua bridge here, now that map name is known
    if (!InitializeLuaBridge(m_CurrentMapName)) {
        std::cerr << "Error: Failed to initialize Lua bridge for map " << m_CurrentMapName << std::endl;
        // Potentially prevent plugin from running further if Lua is critical
    }
}

void CRCBotPlugin::ServerActivate(edict_t *pEdictList, int edictCount, int clientMax) {
    std::cout << "CRCBotPlugin::ServerActivate (Conceptual)" << std::endl;
    // If Lua bridge wasn't initialized in LevelInit, this is another option.
}

void CRCBotPlugin::GameFrame(bool simulating) {
    if (simulating) {
        PluginCycle();
    }
}

void CRCBotPlugin::LevelShutdown() {
    std::cout << "CRCBotPlugin::LevelShutdown (Conceptual)" << std::endl;
    ShutdownLuaBridge();
    m_ControlPoints.clear();
    m_PayloadPaths.clear();
    m_ClassConfigs.clear();
    m_CurrentMapName = "";
}

// --- Plugin Specific Logic ---
void CRCBotPlugin::PluginCycle() {
    // This is where bot logic, Lua calls for updates, etc., would go.
    // For example: CallLuaFunction(m_pLuaState, "RCBot_OnFrameUpdate");
}

// --- Lua Bridge Management ---
bool CRCBotPlugin::FindLuaState() {
    // Conceptual: This is where you'd implement logic to find the game's lua_State*.
    // For many Source mods, Lua is often a singleton within the game server DLL.
    // This might involve:
    // 1. Getting an interface from gameServerFactory.
    // 2. Calling an exported function from that interface that returns lua_State*.
    // 3. Or, if the game exposes a global lua_State* (less common/safe).
    // 4. Signature scanning (more complex, last resort).

    // For this subtask, we'll assume it's found and set manually or by a future step.
    // If a game like Fortress Forever uses gLua (from GMod Lua C API), it might be `gLua->GetState()`
    // THIS IS A PLACEHOLDER - A REAL IMPLEMENTATION IS REQUIRED.
    // m_pLuaState = ... obtain from game ...;

    // If we cannot find it, we must simulate it for now for testing population functions
    if (!m_pLuaState) {
        std::cout << "Warning: Game's lua_State* not found. Creating a new Lua state for testing." << std::endl;
        m_pLuaState = luaL_newstate();
        if (m_pLuaState) {
            luaL_openlibs(m_pLuaState); // Open standard libraries
        } else {
            std::cerr << "Error: Could not create a test Lua state." << std::endl;
            return false;
        }
    }
    return (m_pLuaState != nullptr);
}

bool CRCBotPlugin::InitializeLuaBridge(const char* mapName) {
    if (m_bLuaBridgeInitialized) {
        ShutdownLuaBridge(); // Shutdown first if already initialized (e.g. map change)
    }

    if (!FindLuaState()) {
        std::cerr << "Error: Could not find or initialize Lua state." << std::endl;
        return false;
    }

    RegisterLuaFunctions();

    // Execute a general bot initialization script
    if (!ExecuteLuaScript("scripts/rcbot_init.lua")) { // Path relative to game dir
        std::cerr << "Warning: Failed to execute scripts/rcbot_init.lua" << std::endl;
        // Decide if this is a fatal error
    }

    // Execute map-specific data script
    if (mapName && mapName[0] != '\0') {
        char mapScriptPath[256];
        snprintf(mapScriptPath, sizeof(mapScriptPath), "scripts/maps/data/%s_data.lua", mapName);
        if (!ExecuteLuaScript(mapScriptPath)) {
            std::cerr << "Warning: Failed to execute map-specific script: " << mapScriptPath << std::endl;
        }
    }

    // Populate data structures from Lua
    m_ControlPoints.clear(); // Clear previous map's data
    // Example: Assuming Lua tables like "g_CP_Badlands_Mid", "g_CP_Badlands_Spire" etc.
    // This part needs a more robust way to know which tables to load,
    // perhaps from a main config table loaded by rcbot_init.lua or map_data.lua.
    ControlPointInfo cpTest;
    if (PopulateControlPointInfo(m_pLuaState, cpTest, "g_ControlPoint_TestCP1")) {
         m_ControlPoints.push_back(cpTest);
         std::cout << "Successfully populated g_ControlPoint_TestCP1" << std::endl;
    } else {
         std::cout << "Failed to populate g_ControlPoint_TestCP1" << std::endl;
    }
    // Similarly for PayloadPathInfo and ClassConfigInfo
    // For ClassConfigInfo, you'd typically loop through a list of class config table names.
    ClassConfigInfo classTest;
    if (PopulateClassConfigInfo(m_pLuaState, classTest, "g_ClassConfig_Scout_Test")) {
        m_ClassConfigs.push_back(classTest);
        std::cout << "Successfully populated g_ClassConfig_Scout_Test" << std::endl;
    } else {
        std::cout << "Failed to populate g_ClassConfig_Scout_Test" << std::endl;
    }


    m_bLuaBridgeInitialized = true;
    std::cout << "Lua bridge initialized." << std::endl;
    return true;
}

void CRCBotPlugin::ShutdownLuaBridge() {
    if (m_pLuaState && m_bLuaBridgeInitialized) {
        // If we created the Lua state for testing, we should close it.
        // If it was the game's state, we generally DON'T close it.
        // For this example, assuming if it was created by FindLuaState placeholder, it should be closed.
        // A more robust FindLuaState would tell us if we "own" the state.
        // For now, let's assume if it's the test one, it gets closed.
        // A real FindLuaState() might return a flag or m_pLuaState would be null if a test one wasn't needed.
        // This is tricky. For now, let's just say if it's not the game's actual state, it would be closed.
        // lua_close(m_pLuaState); // Be VERY careful with this if it's the game's shared state!
    }
    m_pLuaState = nullptr;
    m_bLuaBridgeInitialized = false;
    std::cout << "Lua bridge shutdown." << std::endl;
}

bool CRCBotPlugin::ExecuteLuaScript(const char* scriptPath) {
    if (!m_pLuaState) {
        std::cerr << "Error: Lua state not available to execute " << scriptPath << std::endl;
        return false;
    }
    if (luaL_dofile(m_pLuaState, scriptPath) != LUA_OK) {
        const char* luaError = lua_tostring(m_pLuaState, -1);
        std::cerr << "Error executing Lua script '" << scriptPath << "': " << luaError << std::endl;
        lua_pop(m_pLuaState, 1); // Pop error message
        return false;
    }
    std::cout << "Executed Lua script: " << scriptPath << std::endl;
    return true;
}

// --- C++ Functions Exposed to Lua (Static Wrappers) ---
static int Lua_LogMessage(lua_State* L) {
    const char* message = luaL_checkstring(L, 1);
    if (g_pRCBotPlugin) {
        g_pRCBotPlugin->LogMessageToConsole(message);
    }
    return 0; // Number of return values
}

static int Lua_GetGameTime(lua_State* L) {
    if (g_pRCBotPlugin) {
        lua_pushnumber(L, g_pRCBotPlugin->GetCurrentGameTime());
    } else {
        lua_pushnumber(L, 0.0); // Default if plugin instance not found
    }
    return 1; // One return value
}

static int Lua_GetMapName(lua_State* L) {
    if (g_pRCBotPlugin) {
        lua_pushstring(L, g_pRCBotPlugin->GetCurrentMapName());
    } else {
        lua_pushnil(L);
    }
    return 1; // One return value
}

// --- Actual Implementations for Exposed Functions ---
void CRCBotPlugin::LogMessageToConsole(const char* message) {
    // In a real plugin, use engine->ConMsg or similar
    std::cout << "[RCBot Lua]: " << message << std::endl;
    // if (m_pEngineServer) { // Example using a stored engine interface
    //    m_pEngineServer->ConMsg("[RCBot Lua]: %s\n", message);
    // }
}

float CRCBotPlugin::GetCurrentGameTime() const {
    // In a real plugin, get this from gpGlobals->curtime or engine equivalent
    // For now, returning a placeholder.
    // static float time = 0; time += 0.015f; return time;
    return 0.0f; // Placeholder
}

const char* CRCBotPlugin::GetCurrentMapName() const {
    return m_CurrentMapName;
}

// --- Register C++ Functions with Lua ---
void CRCBotPlugin::RegisterLuaFunctions() {
    if (!m_pLuaState) return;

    // Create a global table "RCBot" to act as a namespace
    lua_newtable(m_pLuaState); // Create table RCBot

    lua_pushcfunction(m_pLuaState, Lua_LogMessage);
    lua_setfield(m_pLuaState, -2, "LogMessage"); // RCBot.LogMessage

    lua_pushcfunction(m_pLuaState, Lua_GetGameTime);
    lua_setfield(m_pLuaState, -2, "GetGameTime"); // RCBot.GetGameTime

    lua_pushcfunction(m_pLuaState, Lua_GetMapName);
    lua_setfield(m_pLuaState, -2, "GetMapName"); // RCBot.GetMapName

    lua_setglobal(m_pLuaState, "RCBot"); // Register RCBot table globally

    std::cout << "Registered C++ functions with Lua." << std::endl;
}
