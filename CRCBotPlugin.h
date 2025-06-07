#ifndef CRCBOTPLUGIN_H
#define CRCBOTPLUGIN_H

#include "GameDefines_Placeholder.h" // For CCommand, edict_t, engine interface versions etc.
#include "FFStateStructs.h"      // For BotInfo, TeamID, ClassID, etc.
#include "BotKnowledgeBase.h"    // For m_pKnowledgeBase
#include "FFLuaBridge.h"         // For Lua state and functions
#include "ObjectivePlanner.h"    // For CObjectivePlanner
#include "AIFactory.h"           // For CreateAIModule
#include "CFFPlayer.h"           // For CFFPlayer

#include <vector>
#include <string>
#include <memory> // For std::unique_ptr
#include <map>    // For m_PendingBots

// Forward declare engine interfaces (conceptual, actual SDK headers would provide these)
class IVEngineServer;
class IPlayerInfoManager_Conceptual; // Assuming a conceptual name
class IServerGameClients_Conceptual;
class IBotManager_Conceptual;        // For CreateBot
class IGameEventManager2_Conceptual;
class ICvar_Conceptual;              // Using a distinct name if different from GameDefines
class IEngineTrace_Conceptual;

// Define from plugin_define.h or here if specific
#ifndef INTERFACEVERSION_ISERVERPLUGINCALLBACKS
#define INTERFACEVERSION_ISERVERPLUGINCALLBACKS INTERFACEVERSION_ISERVERPLUGINCALLBACKS_CURRENT
#endif

// Interface for the plugin to expose
class IServerPluginCallbacks {
public:
    virtual ~IServerPluginCallbacks() {};
    virtual bool Load(void* (*interfaceFactory)(const char *pName, int *pReturnCode), void* (*gameServerFactory)(const char *pName, int *pReturnCode)) = 0;
    virtual void Unload(void) = 0;
    virtual void Pause(void) = 0;
    virtual void UnPause(void) = 0;
    virtual const char* GetPluginDescription(void) = 0;
    virtual void LevelInit(char const *pMapName) = 0;
    virtual void ServerActivate(edict_t *pEdictList, int edictCount, int clientMax) = 0;
    virtual void GameFrame(bool simulating) = 0;
    virtual void LevelShutdown(void) = 0;
    virtual void ClientActive(edict_t *pEntity) = 0;
    virtual void ClientDisconnect(edict_t *pEntity) = 0;
    virtual void ClientPutInServer(edict_t *pEntity, char const *playername) = 0;
    virtual void SetCommandClient(int index) = 0;
    virtual void ClientSettingsChanged(edict_t *pEdict) = 0;
    virtual int  ClientConnect(bool *bAllowConnect, edict_t *pEntity, const char *pszName, const char *pszAddress, char *reject, int maxrejectlen) = 0;
    virtual void ClientCommand(edict_t *pEntity, const CCommand &args) = 0;
    virtual void NetworkIDValidated(const char *pszUserName, const char *pszNetworkID) = 0;
    virtual void OnQueryCvarValueFinished(int iCookie, edict_t *pPlayerEntity, int eStatus, const char *pCvarName, const char *pCvarValue) = 0;
    virtual void OnEdictAllocated(edict_t *edict) = 0;
    virtual void OnEdictFreed(edict_t *edict) = 0;
    // For IGameEventManager2
    virtual void FireGameEvent(void *event_conceptual /* IGameEvent *event */) = 0;
};


struct BotInfo {
    edict_t* pEdict = nullptr;
    int userId = 0; // Conceptual UserID from engine event or player info
    std::string name;
    int requestedTeamId = 0;
    std::string requestedClassName;
    int skill = 3;
    bool isActive = false; // Becomes true after ClientPutInServer and full setup

    std::unique_ptr<CFFPlayer> pPlayer;
    std::unique_ptr<CObjectivePlanner> pPlanner;
    std::unique_ptr<CFFBaseAI> pAIModule;

    BotInfo(const std::string& _name, int team, const std::string& className, int _skill)
        : name(_name), requestedTeamId(team), requestedClassName(className), skill(_skill) {}
};

class CRCBotPlugin : public IServerPluginCallbacks /*, public IGameEventListener2_Conceptual_if_needed */ {
public:
    CRCBotPlugin();
    ~CRCBotPlugin() override;

    // IServerPluginCallbacks implementation
    bool Load(void* (*interfaceFactory)(const char *pName, int *pReturnCode), void* (*gameServerFactory)(const char *pName, int *pReturnCode)) override;
    void Unload(void) override;
    void Pause(void) override;
    void UnPause(void) override;
    const char* GetPluginDescription(void) override;
    void LevelInit(char const *pMapName) override;
    void ServerActivate(edict_t *pEdictList, int edictCount, int clientMax) override;
    void GameFrame(bool simulating) override;
    void LevelShutdown(void) override;
    void ClientActive(edict_t *pEntity) override;
    void ClientDisconnect(edict_t *pEntity) override;
    void ClientPutInServer(edict_t *pEntity, char const *playername) override;
    void SetCommandClient(int index) override;
    void ClientSettingsChanged(edict_t *pEdict) override;
    int  ClientConnect(bool *bAllowConnect, edict_t *pEntity, const char *pszName, const char *pszAddress, char *reject, int maxrejectlen) override;
    void ClientCommand(edict_t *pEntity, const CCommand &args) override;
    void NetworkIDValidated(const char *pszUserName, const char *pszNetworkID) override;
    void OnQueryCvarValueFinished(int iCookie, edict_t *pPlayerEntity, int eStatus, const char *pCvarName, const char *pCvarValue) override;
    void OnEdictAllocated(edict_t *edict) override;
    void OnEdictFreed(edict_t *edict) override;

    // For IGameEventManager2 (conceptual, actual event type is IGameEvent*)
    virtual void FireGameEvent(void *event_conceptual);

    // RCBot specific functions
    bool RequestBot(const std::string& name, int teamId, const std::string& className, int skill);
    void RemoveBot(const std::string& nameOrIndex); // Can be name or stringified edict index
    void UpdateAllBots(CUserCmd* usercmds_output_array, int max_usercmds_capacity);

    BotKnowledgeBase* GetKnowledgeBase() { return m_pKnowledgeBase.get(); }
    IVEngineServer* GetEngineServer() { return m_pEngineServer_member; } // Getter for internal use
    lua_State* GetLuaState() { return m_pLuaState_member; }

    void StoreTaskLog(const TaskOutcomeLog& log);
    void SaveTaskLogsToFile();

    // Static callback for ConCommand
    static void RCBot_Add_Cmd_Callback(const CCommand& args);

private:
    bool InitializeLuaBridge();
    void ShutdownLuaBridge();
    void RegisterLuaFunctionsWithPlugin();
    bool LoadMapDataFromLua(const std::string& mapName);
    bool LoadGlobalClassConfigsFromLua();

    void FinalizeBotAddition(BotInfo& botInfo, edict_t* pEdict);
    void UpdatePerceptionSystem_Conceptual(); // Placeholder for world state updates
    void PollGameState_Conceptual();          // Placeholder for direct game state polling

    // Member engine interfaces (conceptual)
    IVEngineServer* m_pEngineServer_member = nullptr;
    IPlayerInfoManager_Conceptual* m_pPlayerInfoManager_member = nullptr;
    IServerGameClients_Conceptual* m_pServerGameClients_member = nullptr;
    IGameEventManager2_Conceptual* m_pGameEventManager_member = nullptr;
    ICvar_Conceptual* m_pCVar_member = nullptr;
    IBotManager_Conceptual* m_pBotManager_member = nullptr; // For engine's CreateBot if used
    IEngineTrace_Conceptual* m_pEngineTrace_member = nullptr; // For raycasts etc.

    std::unique_ptr<BotKnowledgeBase> m_pKnowledgeBase;
    std::vector<std::unique_ptr<BotInfo>> m_ManagedBots;
    std::map<std::string, std::unique_ptr<BotInfo>> m_PendingBots; // Keyed by a temporary ID or name until edict is known

    lua_State* m_pLuaState_member = nullptr; // Lua state for configuration and scripting

    std::vector<TaskOutcomeLog> m_CompletedTaskLogs;
    std::string m_CurrentMapName_Log;
    bool m_bLuaInitialized = false;
    bool m_bPluginLoaded = false; // Tracks if Load was successful
    int m_CommandClientIndex = -1; // For SetCommandClient
};

// Global pointer to the plugin instance (for ConCommand callback and other global access if needed)
// This is often set in Load() or through the EXPOSE macro.
extern CRCBotPlugin g_CRCBotPlugin;


#endif // CRCBOTPLUGIN_H
