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
class CFFPlayer;        // Added forward declaration
struct BotKnowledgeBase;
struct ClassConfigInfo;
struct TaskOutcomeLog;
struct lua_State;
// class IGameEvent; // From SDK's gameeventmanager.h, included above

// Conceptual forward declarations for engine interfaces
class IVEngineServer;
class IPlayerInfoManager;
class IServerGameClients;
class IBotManager;
// class IGameEventManager2; // Included above
class IEngineTrace;
class ICvar;
class CGlobalVarsBase;
// class INavMesh; // For NavMesh loading

#ifndef INTERFACEVERSION_ISERVERPLUGINCALLBACKS
#define INTERFACEVERSION_ISERVERPLUGINCALLBACKS "ISERVERPLUGINCALLBACKS003" // Common version
#endif
#ifndef PLUGIN_INTERFACE_VERSION // If not in iserverplugin.h
#define PLUGIN_INTERFACE_VERSION INTERFACEVERSION_ISERVERPLUGINCALLBACKS
#endif


struct BotInfo {
    edict_t* pEdict;
    int botId;
    std::string name;

    int teamId;
    std::string className;
    int classIdInternal;

    bool isPendingPlayerSlot;
    std::string nameRequest;
    int teamIdRequest;
    std::string classNameRequest;
    int skillLevel;

    std::unique_ptr<CFFPlayer> playerWrapper; // Owns the CFFPlayer instance
    std::unique_ptr<CFFBaseAI> aiModule;
    std::unique_ptr<CObjectivePlanner> objectivePlanner;

    bool isActive;

    // Constructor for pending state
    BotInfo(int id, const std::string& reqName, int reqTeam, const std::string& reqClass, int skill)
        : pEdict(nullptr), botId(id), name(reqName), // name is reqName initially
          teamId(0), className(""), classIdInternal(0),
          isPendingPlayerSlot(true), nameRequest(reqName), teamIdRequest(reqTeam),
          classNameRequest(reqClass), skillLevel(skill),
          playerWrapper(nullptr), aiModule(nullptr), objectivePlanner(nullptr),
          isActive(false) {}

    // Constructor for when edict is known (used by emplace_back in m_ManagedBots if needed)
    BotInfo(edict_t* ed, int id) :
        pEdict(ed), botId(id), teamId(0), classIdInternal(0),
        isPendingPlayerSlot(false), teamIdRequest(0), skillLevel(0), // These might be set later if needed
        playerWrapper(nullptr), aiModule(nullptr), objectivePlanner(nullptr),
        isActive(false) {}


    // Default constructor for map/vector convenience if strictly needed (though emplace_back is better)
    BotInfo() : pEdict(nullptr), botId(-1), teamId(0), classIdInternal(0),
                isPendingPlayerSlot(false), teamIdRequest(0), skillLevel(0),
                playerWrapper(nullptr), aiModule(nullptr), objectivePlanner(nullptr),
                isActive(false) {}

    BotInfo(BotInfo&& other) noexcept = default;
    BotInfo& operator=(BotInfo&& other) noexcept = default;
    BotInfo(const BotInfo&) = delete;
    BotInfo& operator=(const BotInfo&) = delete;
};

class CRCBotPlugin : public IServerPluginCallbacks, public IGameEventListener2 {
public:
    CRCBotPlugin();
    virtual ~CRCBotPlugin();

    // IServerPluginCallbacks
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
    virtual PLUGIN_RESULT ClientCommand(edict_t *pEntity, const void*& args /*const CCommand& args*/) override;
    virtual PLUGIN_RESULT NetworkIDValidated(const char *pszUserName, const char *pszNetworkID) override;
    virtual void OnQueryCvarValueFinished(int iCookie, edict_t *pPlayerEntity, int eStatus, const char *pCvarName, const char *pCvarValue) override;
    virtual void OnEdictAllocated(edict_t *edict) override;
    virtual void OnEdictFreed(const edict_t *edict) override;

    // IGameEventListener2
    virtual void FireGameEvent(IGameEvent *event) override;
    virtual int GetEventDebugID(void) override { return 42; }

    // Bot Management
    int RequestBot(const std::string& name, int team, const std::string& className, int skill);
    void RemoveBot(edict_t* pBotEdict);

    void StoreTaskLog(const TaskOutcomeLog& log);

    // Accessors
    IVEngineServer* GetEngineServer_Instance() const { return m_pEngineServer_member; }
    CGlobalVarsBase* GetGlobals_Instance() const { return m_pGlobals_member; }
    BotKnowledgeBase* GetKnowledgeBase() const { return m_pGlobalKnowledgeBase.get(); }
    lua_State* GetLuaState() const { return m_pLuaState; }
    ICvar* GetCVar_Instance() const { return m_pCVar_member; }


private:
    void FinalizeBotAddition(edict_t* pBotEdict, BotInfo& botInfoFromPending); // Changed signature
    void UpdateAllBots();

    bool InitializeLuaBridge();
    void ShutdownLuaBridge();
    void RegisterLuaFunctionsWithPlugin();
    void LoadMapDataFromLua(const char* pMapName);

    void SaveTaskLogsToFile();

    void UpdatePerceptionSystem_Conceptual();
    void PollGameState_Conceptual();

    std::vector<BotInfo> m_ManagedBots;
    std::map<std::string, BotInfo> m_PendingBots; // Keyed by requested name
    int m_NextBotIdCounter;

    // Engine Interfaces
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
    // ClassConfigs and map objective data are now stored within BotKnowledgeBase

    std::vector<TaskOutcomeLog> m_CompletedTaskLogs;
    bool m_bRegisteredForEvents;

    // Helper
    BotInfo* GetPendingBotByName_Internal(const std::string& name);
};

extern CRCBotPlugin g_CRCBotPlugin; // For EXPOSE_PLUGIN if that's the pattern

#endif // CRC_BOT_PLUGIN_H
