#ifndef CRC_BOT_PLUGIN_H
#define CRC_BOT_PLUGIN_H

// Actual SDK Includes (paths are conceptual relative to a base like fortressforever/src/)
#include "public/eiface.h"           // For CreateInterfaceFn, IServerPluginCallbacks (often includes edict.h)
#include "public/engine/iserverplugin.h" // For PLUGIN_RESULT, QueryCvarCookie_t, EQueryCvarValueStatus
#include "public/igameevents.h"      // For IGameEventListener2 & IGameEvent
// #include "public/edict.h"         // Usually included by eiface.h or iserverplugin.h
// #include "public/mathlib/vector.h" // For Vector - usually brought in by other core headers
// #include "public/mathlib/qangle.h" // For QAngle - usually brought in by other core headers

// Forward declare from SDK if full include not needed here or causes issues
class CCommand;     // From public/tier1/convar.h or similar (e.g. public/convar.h)
class ConCommand;   // From public/tier1/convar.h or similar
struct edict_t;     // Already forward declared or defined via included headers like eiface.h

// Bot Framework Includes
#include <vector>
#include <string>
#include <memory>   // For std::unique_ptr
#include <map>      // For std::map

#include "FFBot_SDKDefines.h" // Our internal enums and mappings

// Forward declarations for our classes (if not fully included)
class CFFBaseAI;
class CObjectivePlanner;
class CFFPlayer;
class BotKnowledgeBase; // Changed from struct to class for consistency if it has methods
struct TaskOutcomeLog;
struct lua_State;

// Engine Interface Forward Declarations (from SDK)
class IVEngineServer;       // From public/engine/ivengineserver.h
class IPlayerInfoManager;   // From game/server/iplayerinfo.h (Fortress Forever specific location might vary)
class IServerGameClients;   // From public/eiface.h (provides server-side client ops)
class IBotManager;          // May be specific to some Source engine versions or games, or not exist.
class IEngineTrace;         // From public/engine/IEngineTrace.h
class ICvar;                // From public/tier1/iconvar.h or public/convar.h
class CGlobalVarsBase;      // From public/globalvars_base.h

// BotInfo structure using SDK types where possible
struct BotInfo {
    edict_t* pEdict;        // Actual edict_t from SDK
    int botId;              // Our internal unique ID for the bot instance
    std::string name;       // Bot's current in-game name

    int teamId;             // Actual SDK team ID (e.g., FF_TEAM_RED, FF_TEAM_BLUE from ff_shareddefs.h or similar)
    FF_BotPlayerClassID internalClassId; // Our internal enum for class logic

    // For pending bots that haven't fully joined yet
    bool isPendingPlayerSlot;   // True if we've requested a fake client and are waiting for ClientPutInServer
    std::string nameRequest;    // Name requested for the bot
    int teamIdRequest;          // Requested team (using SDK TEAM_ defines or our TEAM_ID_BOT_AUTO_ASSIGN)
    std::string classNameRequest; // Requested class as a string (e.g., "soldier", "medic")
    int skillLevel;

    std::unique_ptr<CFFPlayer> playerWrapper;
    std::unique_ptr<CFFBaseAI> aiModule;
    std::unique_ptr<CObjectivePlanner> objectivePlanner;

    bool isActive; // True if bot is fully in game and AI is running

    // Constructor for an already existing edict (e.g. if attaching to existing bot)
    BotInfo(edict_t* ed, int id) :
        pEdict(ed), botId(id), teamId(0 /*TEAM_UNASSIGNED from SDK*/), internalClassId(FF_BOT_CLASS_UNKNOWN),
        isPendingPlayerSlot(false), teamIdRequest(TEAM_ID_BOT_AUTO_ASSIGN), skillLevel(3),
        isActive(false) {}

    // Default constructor for creating BotInfo before edict is known (e.g. for m_PendingBots value)
    BotInfo() : pEdict(nullptr), botId(-1), teamId(0 /*TEAM_UNASSIGNED from SDK*/), internalClassId(FF_BOT_CLASS_UNKNOWN),
                isPendingPlayerSlot(false), teamIdRequest(TEAM_ID_BOT_AUTO_ASSIGN), skillLevel(3),
                isActive(false) {}
};

class CRCBotPlugin : public IServerPluginCallbacks, public IGameEventListener2 {
public:
    CRCBotPlugin();
    virtual ~CRCBotPlugin();

    // IServerPluginCallbacks (signatures from SDK's IServerPlugin.h / eiface.h)
    virtual bool Load(CreateInterfaceFn interfaceFactory, CreateInterfaceFn gameServerFactory);
    virtual void Unload();
    virtual void Pause();
    virtual void UnPause();
    virtual const char *GetPluginDescription();
    virtual void LevelInit(char const *pMapName);
    virtual void ServerActivate(edict_t *pEdictList, int edictCount, int clientMax);
    virtual void GameFrame(bool simulating);
    virtual void LevelShutdown();
    virtual void ClientActive(edict_t *pEntity);
    virtual void ClientDisconnect(edict_t *pEntity);
    virtual void ClientPutInServer(edict_t *pEntity, char const *playername);
    virtual void SetCommandClient(int index); // index of client calling a command on the server
    virtual void ClientSettingsChanged(edict_t *pEdict); // Player changed cvar/settings
    virtual PLUGIN_RESULT ClientConnect(bool *bAllowConnect, edict_t *pEntity, const char *pszName, const char *pszAddress, char *reject, int maxrejectlen);
    virtual PLUGIN_RESULT ClientCommand(edict_t *pEntity, const CCommand &args); // Use SDK's const CCommand&
    virtual PLUGIN_RESULT NetworkIDValidated(const char *pszUserName, const char *pszNetworkID);
    virtual void OnQueryCvarValueFinished(QueryCvarCookie_t iCookie, edict_t *pPlayerEntity, EQueryCvarValueStatus eStatus, const char *pCvarName, const char *pCvarValue);
    virtual void OnEdictAllocated(edict_t *edict);
    virtual void OnEdictFreed(const edict_t *edict); // SDK uses const edict_t*

    // IGameEventListener2 (signatures from SDK's igameevents.h)
    virtual void FireGameEvent(IGameEvent *event); // Use SDK's IGameEvent
    virtual int GetEventDebugID(void) { return 42; } // As per SDK, return an arbitrary integer.

    // Bot Management
    bool RequestBot(const std::string& name, int teamId, const std::string& className, int skill); // teamId uses SDK FF_TEAM_ or our TEAM_ID_BOT_AUTO_ASSIGN
    void RemoveBot(edict_t* pBotEdict); // Remove by edict

    void StoreTaskLog(const TaskOutcomeLog& log); // Assumes TaskOutcomeLog is defined in BotLearningData.h or similar

    // Accessors
    IVEngineServer* GetEngineServer() const { return m_pEngineServer_member; }
    CGlobalVarsBase* GetGlobals() const { return m_pGlobals_member; }
    BotKnowledgeBase* GetKnowledgeBase() const { return m_pGlobalKnowledgeBase.get(); }
    lua_State* GetLuaState() const { return m_pLuaState; }
    ICvar* GetCVar() const { return m_pCVar_member; }


private:
    void FinalizeBotAddition(edict_t* pBotEdict, BotInfo& botDataFromManagedList, const BotInfo& pendingData); // Modified to pass reference to BotInfo in m_ManagedBots
    void UpdateAllBots(); // Called from GameFrame

    bool InitializeLuaBridge();
    void ShutdownLuaBridge();
    void RegisterLuaFunctionsWithPlugin();
    bool LoadGlobalClassConfigsFromLua(); // New, to be called from Load()
    void LoadMapDataFromLua(const char* pMapName);

    void SaveTaskLogsToFile();

    void UpdatePerceptionSystem(); // Renamed from _Conceptual
    void PollGameState();        // Renamed from _Conceptual

    // ConCommand Callback (static)
    static void RCBot_Add_Cmd_Callback(const CCommand& args); // SDK CCommand

    // Bot Storage
    std::vector<BotInfo> m_ManagedBots;
    std::map<std::string, BotInfo> m_PendingBots; // Keyed by requested name, stores BotInfo by value for pending state
    int m_NextBotIdCounter; // For unique botId in BotInfo

    // Engine Interfaces (members)
    IVEngineServer*       m_pEngineServer_member;
    IPlayerInfoManager*   m_pPlayerInfoManager_member; // Actual type from SDK (e.g., from game/server/iplayerinfo.h)
    IServerGameClients* m_pServerGameClients_member; // Actual type from SDK (e.g., from public/eiface.h)
    IBotManager*          m_pBotManager_member;      // Actual type from SDK (if available, e.g. engine/ibotmanager.h)
    IGameEventManager2*   m_pGameEventManager_member;
    IEngineTrace*         m_pEngineTraceClient_member;
    ICvar*                m_pCVar_member;
    CGlobalVarsBase*      m_pGlobals_member;
    // INavMesh*             m_pEngineNavMeshInterface_member; // If using engine's nav mesh

    lua_State* m_pLuaState;
    bool m_bLuaStateCreatedByPlugin; // True if this plugin created the Lua state

    std::unique_ptr<BotKnowledgeBase> m_pGlobalKnowledgeBase;

    std::vector<TaskOutcomeLog> m_CompletedTaskLogs;
    std::string m_sCurrentMapName; // For logging and map-specific data
    bool m_bRegisteredForEvents;
    float m_fNextFullPerceptionScanTime; // For perception throttling

    // ConCommand object (pointer to SDK's ConCommand)
    ConCommand* m_pCmdBotAdd;
};

// Global instance, exposed to the engine
extern CRCBotPlugin g_CRCBotPlugin;

#endif // CRC_BOT_PLUGIN_H
