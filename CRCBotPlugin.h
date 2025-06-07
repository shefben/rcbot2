#ifndef CRC_BOT_PLUGIN_H
#define CRC_BOT_PLUGIN_H

#ifdef _WIN32
#pragma once
#endif

#include <vector>
#include <string>
#include <memory> // For std::unique_ptr
#include <map>    // For m_PendingBots

// Conceptual Source SDK includes - actual paths/names might vary
#include "public/eiface.h"           // For IServerPluginCallbacks, CreateInterfaceFn, PLUGIN_RESULT
// #include "public/edict.h"         // For edict_t (often included by eiface.h)
#include "public/engine/iserverplugin.h" // For PLUGIN_INTERFACE_VERSION (or define it if missing)
#include "public/igameevents.h"      // For IGameEventListener2, IGameEvent

// Forward declarations from our framework
struct CUserCmd;        // Conceptual, or from GameDefines_Placeholder.h / BotTasks.h
class CFFBaseAI;
class CObjectivePlanner;
struct BotKnowledgeBase;
struct ClassConfigInfo;
struct TaskOutcomeLog;
struct lua_State;

// Conceptual forward declarations for engine interfaces
class IVEngineServer;
class IPlayerInfoManager;
class IServerGameClients;
class IBotManager;          // For bot creation/management if available
class IEngineTrace;
class ICvar;
class CGlobalVarsBase;      // For gpGlobals
// class INavMesh;          // Conceptual for engine's nav mesh interface

#ifndef INTERFACEVERSION_ISERVERPLUGINCALLBACKS
#define INTERFACEVERSION_ISERVERPLUGINCALLBACKS "ISERVERPLUGINCALLBACKS003" // Common version
#endif
#ifndef PLUGIN_INTERFACE_VERSION // If not in iserverplugin.h
#define PLUGIN_INTERFACE_VERSION INTERFACEVERSION_ISERVERPLUGINCALLBACKS
#endif


struct BotInfo {
    edict_t* pEdict;    // Engine entity, set upon successful spawn confirmation
    int botId;          // Plugin-internal unique ID
    std::string name;   // Current name of the bot

    int teamId;         // Actual current team ID in game
    std::string className; // Actual current class name in game
    int classIdInternal; // Game-specific internal class ID

    // Requested parameters (used when creating the bot)
    bool isPendingPlayerSlot;   // True if we've asked engine for a slot, waiting for ClientPutInServer
    std::string nameRequest;    // Name used to match in ClientPutInServer
    int teamIdRequest;          // Requested team by user/logic
    std::string classNameRequest; // Requested class by user/logic
    int skillLevel;             // Skill level for this bot

    std::unique_ptr<CFFBaseAI> aiModule;
    std::unique_ptr<CObjectivePlanner> objectivePlanner;
    std::unique_ptr<CFFPlayer> playerWrapper; // Wrapper around pEdict for easier AI interaction

    bool isActive;      // Is this bot fully initialized and its AI running?

    // Constructor for initial request (before edict is known)
    BotInfo(int id, const std::string& reqName, int reqTeam, const std::string& reqClass, int skill)
        : pEdict(nullptr), botId(id), name(reqName), // Initially set name to reqName
          teamId(0), className(""), classIdInternal(0), // Actuals are unknown yet
          isPendingPlayerSlot(true), nameRequest(reqName), teamIdRequest(reqTeam),
          classNameRequest(reqClass), skillLevel(skill),
          aiModule(nullptr), objectivePlanner(nullptr), playerWrapper(nullptr),
          isActive(false) {}

    // Default constructor for map/vector convenience if needed (though emplace_back with args is better)
    BotInfo() : pEdict(nullptr), botId(-1), teamId(0), classIdInternal(0),
                isPendingPlayerSlot(false), teamIdRequest(0), skillLevel(0),
                isActive(false) {}


    // Move semantics for unique_ptr members
    BotInfo(BotInfo&& other) noexcept = default;
    BotInfo& operator=(BotInfo&& other) noexcept = default;

    // Prevent copying
    BotInfo(const BotInfo&) = delete;
    BotInfo& operator=(const BotInfo&) = delete;
};

class CRCBotPlugin : public IServerPluginCallbacks, public IGameEventListener2 {
public:
    CRCBotPlugin();
    virtual ~CRCBotPlugin();

    // IServerPluginCallbacks methods
    virtual bool Load(CreateInterfaceFn interfaceFactory, CreateInterfaceFn gameServerFactory) override;
    virtual void Unload() override;
    virtual void Pause() override;
    virtual void UnPause() override;
    virtual const char *GetPluginDescription() override;
    virtual void LevelInit(char const *pMapName) override;
    virtual void ServerActivate(edict_t *pEdictList, int edictCount, int clientMax) override;
    virtual void GameFrame(bool simulating) override;
    virtual void LevelShutdown() override;
    virtual void ClientActive(edict_t *pEntity) override;
    virtual void ClientDisconnect(edict_t *pEntity) override;
    virtual void ClientPutInServer(edict_t *pEntity, char const *playername) override;
    virtual void SetCommandClient(int index) override;
    virtual void ClientSettingsChanged(edict_t *pEdict) override;
    virtual PLUGIN_RESULT ClientConnect(bool *bAllowConnect, edict_t *pEntity, const char *pszName, const char *pszAddress, char *reject, int maxrejectlen) override;
    virtual PLUGIN_RESULT ClientCommand(edict_t *pEntity, const void*& args /*const CCommand& args*/) override; // Use const CCommand& if available
    virtual PLUGIN_RESULT NetworkIDValidated(const char *pszUserName, const char *pszNetworkID) override;
    virtual void OnQueryCvarValueFinished(int iCookie, edict_t *pPlayerEntity, int eStatus, const char *pCvarName, const char *pCvarValue) override;
    virtual void OnEdictAllocated(edict_t *edict) override;
    virtual void OnEdictFreed(const edict_t *edict) override;

    // IGameEventListener2 method
    virtual void FireGameEvent(IGameEvent *event) override;
    // virtual int GetEventDebugID(void) override { return 42; } // Optional for debugging

    // Bot Management
    int RequestBot(const std::string& name, int team, const std::string& className, int skill);
    void RemoveBot(edict_t* pBotEdict); // Can also be by botId if more convenient internally

    // Logging callback for CObjectivePlanner
    void StoreTaskLog(const TaskOutcomeLog& log);

    // Accessors for global interfaces (if needed by other parts of bot framework directly)
    IVEngineServer* GetEngineServer_Instance() { return m_pEngineServer_member; } // Suffix to avoid clash with globals
    CGlobalVarsBase* GetGlobals_Instance() { return m_pGlobals_member; }
    ICvar* GetCVar_Instance() { return m_pCVar_member; }
    IEngineTrace* GetEngineTrace_Instance() { return m_pEngineTraceClient_member; }
    // INavMesh* GetEngineNavMesh_Instance() { return m_pEngineNavMeshInterface_member; }


private:
    void FinalizeBotAddition(edict_t* pBotEdict, BotInfo& botInfo); // Takes reference to BotInfo in m_ManagedBots
    void UpdateAllBots();

    bool InitializeLuaBridge();
    void ShutdownLuaBridge();
    void RegisterLuaFunctionsWithPlugin();
    void LoadMapDataFromLua(const char* pMapName);

    void SaveTaskLogsToFile();

    // Conceptual methods for game state updates
    void UpdatePerceptionSystem_Conceptual();
    void PollGameState_Conceptual();

    // BotInfo storage: m_PendingBots for requests before edict exists, m_ManagedBots for active bots.
    std::vector<BotInfo> m_ManagedBots; // Store all BotInfo, using flags to denote state
    int m_NextBotIdCounter;

    // Engine Interfaces (member storage pattern)
    IVEngineServer*       m_pEngineServer_member;
    IPlayerInfoManager*   m_pPlayerInfoManager_member;
    IServerGameClients* m_pServerGameClients_member;
    IBotManager*          m_pBotManager_member;
    IGameEventManager2*   m_pGameEventManager_member;
    IEngineTrace*         m_pEngineTraceClient_member;
    ICvar*                m_pCVar_member;
    CGlobalVarsBase*      m_pGlobals_member;
    // INavMesh*             m_pEngineNavMeshInterface_member;

    lua_State* m_pLuaState;
    bool m_bLuaStateCreatedByPlugin;

    std::unique_ptr<BotKnowledgeBase> m_pGlobalKnowledgeBase;
    // Data like ClassConfigs, ControlPoints are now stored within BotKnowledgeBase after loading
    // std::vector<ClassConfigInfo> m_GlobalClassConfigs;
    // std::vector<ControlPointInfo> m_MapControlPoints;
    // std::vector<PayloadPathInfo> m_MapPayloadPaths;

    std::vector<TaskOutcomeLog> m_CompletedTaskLogs;
    bool m_bRegisteredForEvents;

    // Helper to find BotInfo
    BotInfo* GetBotInfoByEdict_Internal(edict_t* pEdict);
    BotInfo* GetBotInfoByName_Pending(const std::string& name);
};

// If using EXPOSE_SINGLE_INTERFACE_GLOBALVAR, this global instance is needed.
// extern CRCBotPlugin g_CRCBotPlugin;

#endif // CRC_BOT_PLUGIN_H
