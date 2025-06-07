#ifndef RCBOT_PLUGIN_H
#define RCBOT_PLUGIN_H

#ifdef _WIN32
#pragma once
#endif

// Conceptual Source SDK includes
#include "public/eiface.h"
#include "public/engine/iserverplugin.h"

// Standard library includes
#include <vector>
#include <string>
#include <memory> // For std::unique_ptr

// Project headers
#include "BotTasks.h"
#include "BotLearningData.h" // For TaskOutcomeLog

// Forward declarations
class CFFBaseAI;
class CObjectivePlanner;
struct BotKnowledgeBase;
struct ClassConfigInfo;
struct lua_State;
// struct edict_t; // From eiface.h
// struct CUserCmd; // From BotTasks.h or conceptual SDK

struct BotInfo {
    edict_t* pEdict;
    int botId;
    std::string name;
    int teamId;
    std::string classNameRequest;
    int internalClassId;
    std::unique_ptr<CFFBaseAI> aiModule;
    std::unique_ptr<CObjectivePlanner> objectivePlanner;
    bool isActive;
    bool isPendingSpawn;

    BotInfo(int id, const std::string& botName, int team, const std::string& clsName)
        : pEdict(nullptr), botId(id), name(botName), teamId(team), classNameRequest(clsName),
          internalClassId(0), aiModule(nullptr), objectivePlanner(nullptr),
          isActive(false), isPendingSpawn(true) {}

    BotInfo(BotInfo&& other) noexcept = default;
    BotInfo& operator=(BotInfo&& other) noexcept = default;
    BotInfo(const BotInfo&) = delete;
    BotInfo& operator=(const BotInfo&) = delete;
};

class CRCBotPlugin : public IServerPluginCallbacks {
public:
    CRCBotPlugin();
    virtual ~CRCBotPlugin();

    // IServerPluginCallbacks methods
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
    virtual void SetCommandClient(int index);
    virtual void ClientSettingsChanged(edict_t *pEdict);
    virtual PLUGIN_RESULT ClientConnect(bool *bAllowConnect, edict_t *pEntity, const char *pszName, const char *pszAddress, char *reject, int maxrejectlen);
    virtual PLUGIN_RESULT ClientCommand(edict_t *pEntity, const void*& args /* const CCommand &args */);
    virtual PLUGIN_RESULT NetworkIDValidated(const char *pszUserName, const char *pszNetworkID);
    virtual void OnQueryCvarValueFinished(int iCookie, edict_t *pPlayerEntity, int eStatus, const char *pCvarName, const char *pCvarValue);
    virtual void OnEdictAllocated(edict_t *edict);
    virtual void OnEdictFreed(const edict_t *edict);

    // Bot Management
    int RequestBot(const std::string& name, int teamId, const std::string& className, int skill = 1);
    void FinalizeBotAddition(edict_t* pBotEdict, BotInfo& botInfo);
    void RemoveBot(edict_t* pBotEdict);
    void UpdateAllBots();

    // Logging - made public for CObjectivePlanner to call
    void StoreTaskLog(const TaskOutcomeLog& log);

private:
    std::vector<BotInfo> m_ManagedBots;
    int m_NextBotIdCounter;

    lua_State* m_pLuaState;
    bool InitializeLuaBridge();
    void ShutdownLuaBridge();
    void RegisterLuaFunctionsWithPlugin();

    std::unique_ptr<BotKnowledgeBase> m_pGlobalKnowledgeBase;
    std::vector<ClassConfigInfo> m_GlobalClassConfigs;
    std::vector<ControlPointInfo> m_MapControlPoints;
    std::vector<PayloadPathInfo> m_MapPayloadPaths;

    // Logging
    std::vector<TaskOutcomeLog> m_CompletedTaskLogs;
    void SaveTaskLogsToFile(); // Called by LevelShutdown or Unload

    BotInfo* GetBotInfoByEdict(edict_t* pEdict);
    BotInfo* GetPendingBotInfoByName(const char* name);
    void LoadMapDataFromLua(const char* mapName);
};

#endif // RCBOT_PLUGIN_H
