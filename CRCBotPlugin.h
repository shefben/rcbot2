#ifndef RCBOT_PLUGIN_H
#define RCBOT_PLUGIN_H

#ifdef _WIN32
#pragma once
#endif

#include "ff_state_structs.h"
#include "BotTasks.h"         // For HighLevelTask, SubTask
#include <vector>
#include <string>
#include <map>
#include <memory> // For std::unique_ptr

// Forward declarations
class CreateInterfaceFn;
class IServerPluginCallbacks;
struct edict_t;
class CCommand;
struct lua_State;
class CFFBaseAI;             // Forward declare base AI class
class CObjectivePlanner;     // Forward declare planner
struct BotKnowledgeBase;     // Forward declare (defined in FFBaseAI.h or similar)


// Engine interface placeholders
class IVEngineServer;
class IServerGameClients;
class CGlobalVarsBase;

// Global pointer to the plugin instance
class CRCBotPlugin;
extern CRCBotPlugin* g_pRCBotPlugin;

// Structure to hold info about managed bots
struct BotInfo {
    edict_t* pEdict; // Engine's representation of the bot player
    std::string name;
    int assignedTeamId;    // Team ID the bot is currently assigned to (e.g., 2 for RED, 3 for BLUE in TF2 terms)
    int desiredClassId;    // Game-specific class ID (e.g., TF_CLASS_SCOUT)
    std::string className; // Human-readable class name
    int skillLevel;
    std::unique_ptr<CFFBaseAI> aiModule;
    std::unique_ptr<CObjectivePlanner> objectivePlanner;
    bool isPendingSpawn; // Flag to indicate if bot is waiting for engine to spawn it after commands

    BotInfo() :
        pEdict(nullptr),
        assignedTeamId(0), // 0 might mean spectator or unassigned
        desiredClassId(0),
        skillLevel(1),
        isPendingSpawn(false)
         {}
};


class CRCBotPlugin // : public IServerPluginCallbacks // Conceptually
{
public:
    CRCBotPlugin();
    virtual ~CRCBotPlugin();

    // --- IServerPluginCallbacks (Conceptual Methods) ---
    virtual bool Load(CreateInterfaceFn interfaceFactory, CreateInterfaceFn gameServerFactory);
    virtual void Unload();
    // ... other IServerPluginCallbacks methods ...
    virtual void LevelInit(char const *pMapName);
    virtual void ServerActivate(edict_t *pEdictList, int edictCount, int clientMax);
    virtual void GameFrame(bool simulating);
    virtual void LevelShutdown();
    // ClientPutInServer is important for finalizing bot setup after engine spawns the entity.
    virtual void ClientPutInServer(edict_t *pEntity, char const *playername);
    virtual void ClientDisconnect(edict_t *pEntity);


    // --- Bot Creation/Management ---
    // Method called by the rcbot_add_bot command handler
    bool RequestNewBot(const char* desiredTeamStr, const char* desiredClassStr, const char* botNameStr, int skill);

    // Called from ClientPutInServer when a bot we requested actually spawns
    void FinalizeBotSetup(edict_t* pBotEdict, const BotInfo& pendingBotInfo);

    // Called when a managed bot disconnects or is kicked
    void OnBotLeaveGame(edict_t* pBotEdict);


    // --- Plugin Specific Logic ---
    void PluginCycle();

    // --- Lua Bridge Management ---
    bool InitializeLuaBridge(const char* mapName);
    void ShutdownLuaBridge();
    bool ExecuteLuaScript(const char* scriptPath);
    void RegisterLuaFunctions();

    // --- C++ functions to be exposed to Lua ---
    void LogMessageToConsole(const char* message);
    float GetCurrentGameTime() const;
    const char* GetCurrentMapName() const;

    // --- Engine Helper Accessors (Conceptual) ---
    IVEngineServer* GetEngineServer() { return m_pEngineServer; }
    CGlobalVarsBase* GetGlobals() { return m_pGlobals; }
    int GetTeamPlayerCount(int teamId) const; // Renamed for clarity
    void AssignBotToTeam(edict_t* pBotEdict, int teamId, const char* desiredClassStr); // Helper


private:
    bool FindLuaState();
    // Conceptual: Actually makes the bot entity run the command (e.g., via engine->ClientCommand)
    void EngineIssueBotCommand(edict_t* pBotEdict, const char* commandFormat, ...);

    // --- Lua State ---
    lua_State* m_pLuaState;
    bool m_bLuaBridgeInitialized;

    // --- Engine Interfaces (example) ---
    IVEngineServer* m_pEngineServer;
    IServerGameClients* m_pServerGameClients;
    CGlobalVarsBase* m_pGlobals;

    // --- Game State Data Storage ---
    std::vector<ControlPointInfo> m_ControlPoints;
    std::vector<PayloadPathInfo>  m_PayloadPaths;
    std::vector<ClassConfigInfo>  m_ClassConfigs;
    std::unique_ptr<BotKnowledgeBase> m_pKnowledgeBase; // Owns the KB

    const char* m_CurrentMapName;

    // --- Bot Management ---
    // Using edict_t* as key might be problematic if edicts are reused quickly.
    // An internal, persistent bot ID might be better long-term.
    std::map<edict_t*, BotInfo> m_ManagedBots;
    // Store bots that have been requested but not yet seen by ClientPutInServer
    std::vector<BotInfo> m_PendingBots;
    int m_NextBotUniqueId; // For generating unique bot names if not provided
};

#endif // RCBOT_PLUGIN_H
