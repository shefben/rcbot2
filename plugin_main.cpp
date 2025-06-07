#include "plugin_define.h"
#include "LuaBridge.h" // For Lua initialization and script execution
#include <stdio.h> // For snprintf

// Define actual global engine interface pointers
IVEngineServer* engine = nullptr; // Also used by LuaBridge.cpp
ICvar* cvar = nullptr;

// Forward declaration for the concommand callback
void TestLuaRunScript_Callback(const CCommand_Placeholder &command);

// Declare the ConCommand
ConCommand_Placeholder_Static g_TestLuaRunScriptCmd(
    "test_lua_run_script",
    "Executes a test Lua script. Usage: test_lua_run_script <script_filename_without_lua_extension>",
    0, // No special flags for now
    TestLuaRunScript_Callback
);

class CSimplePlugin : public IServerPluginCallbacks {
public:
    CSimplePlugin() {};
    ~CSimplePlugin() override {};

    // IServerPluginCallbacks implementation
    bool Load(CreateInterfaceFn interfaceFactory, CreateInterfaceFn gameServerFactory) override {
        engine = (IVEngineServer*)interfaceFactory(IVEngineServer::GetInterfaceVersion(), nullptr);
        if (!engine) {
            // Cannot use engine->Con_Printf here as engine is null
            printf("CSimplePlugin::Load: Failed to get IVEngineServer interface!\n");
            return false;
        }
        LuaBridge::engine = engine; // Provide engine pointer to LuaBridge

        cvar = (ICvar*)interfaceFactory(ICvar::GetInterfaceVersion(), nullptr);
        if (!cvar) {
            engine->Con_Printf("CSimplePlugin::Load: Failed to get ICvar interface!\n");
            // Not necessarily fatal, but can't register concommands
        } else {
            cvar->RegisterConCommand(&g_TestLuaRunScriptCmd);
            engine->Con_Printf("CSimplePlugin::Load: Registered 'test_lua_run_script' concommand.\n");
        }

        engine->Con_Printf("CSimplePlugin::Load: Plugin loaded successfully.\n");
        return true;
    }

    void Unload(void) override {
        if (cvar) {
            cvar->UnregisterConCommand(&g_TestLuaRunScriptCmd);
            engine->Con_Printf("CSimplePlugin::Unload: Unregistered 'test_lua_run_script' concommand.\n");
        }
        LuaBridge::ShutdownLua(); // Ensure Lua is shut down before engine pointer becomes invalid
        engine->Con_Printf("CSimplePlugin::Unload: Plugin unloaded.\n");
        engine = nullptr;
        LuaBridge::engine = nullptr;
        cvar = nullptr;
    }

    void Pause(void) override {
        if (engine) engine->Con_Printf("CSimplePlugin::Pause\n");
    }

    void UnPause(void) override {
        if (engine) engine->Con_Printf("CSimplePlugin::UnPause\n");
    }

    const char* GetPluginDescription(void) override {
        return "Simple Lua Plugin v1.0";
    }

    void LevelInit(char const *pMapName) override {
        if (engine) engine->Con_Printf("CSimplePlugin::LevelInit: Map %s\n", pMapName);
        LuaBridge::InitializeLua();
    }

    void ServerActivate(void *pEdictList, int edictCount, int clientMax) override {
        if (engine) engine->Con_Printf("CSimplePlugin::ServerActivate\n");
    }

    void GameFrame(bool simulating) override {
        // Called each frame
        // Could be used for Lua ticks if necessary: if (g_pLuaState) { /* call a Lua tick function */ }
    }

    void LevelShutdown(void) override {
        if (engine) engine->Con_Printf("CSimplePlugin::LevelShutdown\n");
        LuaBridge::ShutdownLua();
    }

    void ClientActive(void *pEntity) override {}
    void ClientDisconnect(void *pEntity) override {}
    void ClientPutInServer(void *pEntity, char const *playername) override {}
    void SetCommandClient(int index) override {}
    void ClientSettingsChanged(void *pEdict) override {}
    int ClientConnect(bool *bAllowConnect, void *pEntity, const char *pszName, const char *pszAddress, char *reject, int maxrejectlen) override {
        *bAllowConnect = true;
        return 0;
    }
    void ClientCommand(void *pEntity, const void *&cmd) override {} // cmd is CCommand
    void NetworkIDValidated(const char *pszUserName, const char *pszNetworkID) override {}
    void OnQueryCvarValueFinished(int iCookie, void *pPlayerEntity, int eStatus, const char *pCvarName, const char *pCvarValue) override {}
    void OnEdictAllocated(void *edict) override {}
    void OnEdictFreed(void *edict) override {}
};

// Expose the plugin
PLUGIN_EXPOSE(CSimplePlugin, IServerPluginCallbacks);

// Implementation for the concommand callback
void TestLuaRunScript_Callback(const CCommand_Placeholder &command) {
    if (!engine) return;

    if (command.ArgC() < 2) {
        engine->Con_Printf("Usage: %s <script_filename_no_ext>\n", command.Arg(0));
        return;
    }

    const char* script_basename = command.Arg(1);
    char script_path[256]; // Adjust size as needed

    // Assuming scripts are in a 'lua_scripts' directory relative to game root.
    // This path construction is conceptual. In a real Source plugin, you might get game dir via engine.
    // For now, let's assume the CWD or a known relative path works, or scripts are in `game_root/lua_scripts/`
    // A common place for game-specific scripts is `[game_directory]/lua/` or `[game_directory]/scripts/lua/`
    // For this example, let's assume `lua_scripts/` in the same directory as the DLL or game executable path.
    // This is a simplification; robust path handling is needed in a real plugin.

#ifdef _WIN32
    _snprintf(script_path, sizeof(script_path) - 1, "lua_scripts/%s.lua", script_basename);
#else
    snprintf(script_path, sizeof(script_path) - 1, "lua_scripts/%s.lua", script_basename);
#endif
    script_path[sizeof(script_path) - 1] = '\0'; // Ensure null termination

    engine->Con_Printf("Attempting to run script: %s\n", script_path);
    LuaBridge::ExecuteLuaScript(script_path);
}
