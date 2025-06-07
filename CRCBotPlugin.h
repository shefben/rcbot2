#ifndef RCBOT_PLUGIN_H
#define RCBOT_PLUGIN_H

#ifdef _WIN32
#pragma once
#endif

#include "ff_state_structs.h" // From previous subtask
#include <vector>
#include <string>

// Forward declarations for Source Engine types (minimal for this task)
class CreateInterfaceFn;
class IServerPluginCallbacks; // Assuming this is the base interface
struct edict_t;
class CCommand;
// Forward declaration for lua_State
struct lua_State;

// Conceptual: Engine interfaces would be properly included from SDK
class IVEngineServer;
// extern IVEngineServer *engine; // Typically a global provided by engine interface

// Global pointer to the plugin instance for Lua C functions to access plugin members
class CRCBotPlugin;
extern CRCBotPlugin* g_pRCBotPlugin;


class CRCBotPlugin // : public IServerPluginCallbacks // Conceptually
{
public:
    CRCBotPlugin();
    virtual ~CRCBotPlugin();

    // --- IServerPluginCallbacks (Conceptual Methods) ---
    virtual bool Load(CreateInterfaceFn interfaceFactory, CreateInterfaceFn gameServerFactory);
    virtual void Unload();
    virtual void Pause();
    virtual void UnPause();
    virtual const char* GetPluginDescription();
    virtual void LevelInit(char const *pMapName);
    virtual void ServerActivate(edict_t *pEdictList, int edictCount, int clientMax);
    virtual void GameFrame(bool simulating);
    virtual void LevelShutdown();
    // ... other IServerPluginCallbacks methods would be declared here

    // --- Plugin Specific Logic ---
    void PluginCycle(); // Called by GameFrame if simulating

    // --- Lua Bridge Management ---
    bool InitializeLuaBridge(const char* mapName); // Pass map name for map-specific scripts
    void ShutdownLuaBridge();
    bool ExecuteLuaScript(const char* scriptPath);
    void RegisterLuaFunctions();

    // --- C++ functions to be exposed to Lua (actual implementation) ---
    void LogMessageToConsole(const char* message);
    float GetCurrentGameTime() const;
    const char* GetCurrentMapName() const;


private:
    bool FindLuaState(); // Internal helper to locate lua_State*

    // --- Lua State ---
    lua_State* m_pLuaState;
    bool m_bLuaBridgeInitialized;

    // --- Engine Interfaces (example) ---
    IVEngineServer* m_pEngineServer;
    // Add other interfaces like IGameEventManager2, ICvar, etc. as needed

    // --- Game State Data Storage ---
    std::vector<ControlPointInfo> m_ControlPoints;
    std::vector<PayloadPathInfo>  m_PayloadPaths; // Assuming a map might have multiple paths
    std::vector<ClassConfigInfo>  m_ClassConfigs; // For different bot classes/game classes

    const char* m_CurrentMapName; // Store current map name
};

#endif // RCBOT_PLUGIN_H
