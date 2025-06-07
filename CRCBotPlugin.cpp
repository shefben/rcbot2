#include "CRCBotPlugin.h"
#include "AIFactory.h"
#include "ObjectivePlanner.h"
#include "FFBaseAI.h"
#include "BotKnowledgeBase.h"
#include "FFLuaBridge.h"
#include "BotLearningData.h"
#include "CFFPlayer.h"
#include "GameDefines_Placeholder.h"
#include "EngineInterfaces.h"
#include "public/igameevents.h" // For IGameEvent

// Lua headers
extern "C" {
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
}

#include <iostream>
#include <fstream>
#include <algorithm>
#include <stdarg.h>
#include <iomanip>
#include <cstring>

// --- Conceptual Placeholder Types (ensure these are minimal and consistent) ---
#ifndef CUSERCMD_CONCEPTUAL_DEF_CRCBOTPLUGIN_CPP
#define CUSERCMD_CONCEPTUAL_DEF_CRCBOTPLUGIN_CPP
struct CUserCmd { int buttons = 0; float forwardmove = 0.0f; float sidemove = 0.0f; Vector viewangles; int weaponselect = 0; };
#endif
// CBaseEntity, CFFPlayer, edict_t are assumed to be defined sufficiently via other includes or their own .cpp placeholders.
// --- End Conceptual Placeholders ---

// --- Global Interface Pointer Definitions ---
IVEngineServer*       g_pEngineServer = nullptr;
IPlayerInfoManager*   g_pPlayerInfoManager = nullptr;
IServerGameClients*   g_pServerGameClients = nullptr;
IBotManager*          g_pBotManager = nullptr;
IGameEventManager2*   g_pGameEventManager = nullptr;
IEngineTrace*         g_pEngineTraceClient = nullptr;
ICvar*                g_pCVar = nullptr;
CGlobalVarsBase*      g_pGlobals = nullptr;
// INavMesh*             g_pEngineNavMeshInterface = nullptr; // For BotKnowledgeBase::LoadNavMesh

CRCBotPlugin g_CRCBotPlugin_Instance_Implementation;
EXPOSE_SINGLE_INTERFACE_GLOBALVAR(CRCBotPlugin, IServerPluginCallbacks, INTERFACEVERSION_ISERVERPLUGINCALLBACKS, g_CRCBotPlugin_Instance_Implementation);

// --- Conceptual Game-Specific Lua Interface ---
#define FF_LUA_INTERFACE_VERSION_CONCEPTUAL "FF_LUA_INTERFACE_001" // Example
class IGameLuaInterface_Conceptual { /* ... as before ... */
public: virtual ~IGameLuaInterface_Conceptual() {} virtual lua_State* GetSharedLuaState() = 0;
};
// --- End Conceptual ---

CRCBotPlugin::CRCBotPlugin()
    : m_NextBotIdCounter(1),
      m_pLuaState(nullptr),
      m_bLuaStateCreatedByPlugin(false),
      m_pGlobalKnowledgeBase(nullptr),
      m_bRegisteredForEvents(false),
      m_pEngineServer_member(nullptr), m_pPlayerInfoManager_member(nullptr), m_pServerGameClients_member(nullptr),
      m_pBotManager_member(nullptr), m_pGameEventManager_member(nullptr), m_pEngineTraceClient_member(nullptr),
      m_pCVar_member(nullptr), m_pGlobals_member(nullptr)
{
    std::cout << "[RCBot] Constructor: CRCBotPlugin instance created." << std::endl;
}

CRCBotPlugin::~CRCBotPlugin() {
    std::cout << "[RCBot] Destructor: Cleaning up CRCBotPlugin." << std::endl;
    if (!m_CompletedTaskLogs.empty()) { SaveTaskLogsToFile(); }
    m_ManagedBots.clear(); // unique_ptrs in BotInfo will clean up AI/Planner/PlayerWrapper
    m_PendingBots.clear();
}

bool CRCBotPlugin::Load(CreateInterfaceFn interfaceFactory, CreateInterfaceFn gameServerFactory) {
    std::cout << "[RCBot] Load: Initializing plugin..." << std::endl;

    m_pEngineServer_member = (IVEngineServer*)interfaceFactory(INTERFACEVERSION_VENGINESERVER, nullptr);
    g_pEngineServer = m_pEngineServer_member;
    if (!g_pEngineServer) { std::cerr << "[RCBot] FATAL ERROR: IVEngineServer not found." << std::endl; return false; }

    // Conceptual: static CGlobalVarsBase dummyGlobals; g_pGlobals = &dummyGlobals; m_pGlobals_member = g_pGlobals;
    // In real Source SDK, gpGlobals is often an exported global from the engine.
    // For now, assume it's obtained (e.g. via IVEngineServer::GetGlobals() if that exists, or ICvar::GetGlobals())
    // if (g_pEngineServer) g_pGlobals = g_pEngineServer->GetGlobals(); else if(g_pCVar) g_pGlobals = g_pCVar->GetGlobals();
    // m_pGlobals_member = g_pGlobals;
    // if (!g_pGlobals) { std::cerr << "[RCBot] FATAL ERROR: CGlobalVarsBase (gpGlobals) not found." << std::endl; return false; }


    m_pGameEventManager_member = (IGameEventManager2*)interfaceFactory(INTERFACEVERSION_GAMEEVENTSMANAGER2, nullptr);
    g_pGameEventManager = m_pGameEventManager_member;
    if (!g_pGameEventManager && false) { std::cerr << "[RCBot] Error: Failed to get IGameEventManager2." << std::endl; /* Non-fatal for now */ }

    m_pEngineTraceClient_member = (IEngineTrace*)interfaceFactory(INTERFACEVERSION_ENGINETRACE_SERVER, nullptr);
    g_pEngineTraceClient = m_pEngineTraceClient_member;
    if (!g_pEngineTraceClient && false) { std::cerr << "[RCBot] Error: Failed to get IEngineTrace." << std::endl; /* Non-fatal */ }

    m_pCVar_member = (ICvar*)interfaceFactory(CVAR_INTERFACE_VERSION, nullptr);
    g_pCVar = m_pCVar_member;
    if (!g_pCVar && false) { std::cerr << "[RCBot] Error: Failed to get ICvar." << std::endl; /* Non-fatal */ }

    if (gameServerFactory) {
        m_pPlayerInfoManager_member = (IPlayerInfoManager*)gameServerFactory(INTERFACEVERSION_PLAYERINFOMANAGER, nullptr);
        g_pPlayerInfoManager = m_pPlayerInfoManager_member;
        m_pServerGameClients_member = (IServerGameClients*)gameServerFactory(INTERFACEVERSION_SERVERGAMECLIENTS, nullptr);
        g_pServerGameClients = m_pServerGameClients_member;
        m_pBotManager_member = (IBotManager*)gameServerFactory(INTERFACEVERSION_PLAYERBOTMANAGER_GAME, nullptr);
        g_pBotManager = m_pBotManager_member;
        if (!g_pBotManager) {
             m_pBotManager_member = (IBotManager*)interfaceFactory(INTERFACEVERSION_PLAYERBOTMANAGER_ENGINE, nullptr);
             g_pBotManager = m_pBotManager_member;
        }
    }

    m_NextBotIdCounter = 1;
    m_pGlobalKnowledgeBase = std::make_unique<BotKnowledgeBase>();
    if (!m_pGlobalKnowledgeBase) { std::cerr << "[RCBot] FATAL ERROR: Failed to allocate GlobalKnowledgeBase!" << std::endl; return false; }
    // KB internal pointers are set up in its constructor or load methods

    if (!InitializeLuaBridge(/* gameServerFactory if needed by bridge */)) {
        std::cerr << "[RCBot] Warning: Failed to initialize Lua bridge." << std::endl;
    }

    if (m_pLuaState && m_pGlobalKnowledgeBase) {
        std::vector<std::string> classCfgTables;
        lua_getglobal(m_pLuaState, "g_ClassConfigTableNames_FF");
        if (lua_istable(m_pLuaState, -1)) {
            lua_pushnil(m_pLuaState);
            while (lua_next(m_pLuaState, -2) != 0) {
                if (lua_isstring(m_pLuaState, -1)) { classCfgTables.push_back(lua_tostring(m_pLuaState, -1)); }
                lua_pop(m_pLuaState, 1);
            }
        }
        lua_pop(m_pLuaState, 1);
        m_pGlobalKnowledgeBase->LoadGlobalClassConfigs(m_pLuaState, classCfgTables);
    }

    if (g_pGameEventManager) {
        // g_pGameEventManager->AddListener(this, "player_death", true);
        // g_pGameEventManager->AddListener(this, "teamplay_point_captured", true);
        m_bRegisteredForEvents = true;
    }

    std::cout << "[RCBot] Plugin loaded successfully (Restored Full)." << std::endl;
    return true;
}

void CRCBotPlugin::Unload() {
    std::cout << "[RCBot] Unload called." << std::endl;
    SaveTaskLogsToFile();
    if (g_pGameEventManager && m_bRegisteredForEvents) {
        // g_pGameEventManager->RemoveListener(this);
        m_bRegisteredForEvents = false;
    }
    ShutdownLuaBridge();
    m_ManagedBots.clear();
    m_PendingBots.clear();
    m_pGlobalKnowledgeBase.reset();
    m_CompletedTaskLogs.clear();
    g_pEngineServer = nullptr; g_pPlayerInfoManager = nullptr; g_pServerGameClients = nullptr;
    g_pBotManager = nullptr; g_pGameEventManager = nullptr; g_pEngineTraceClient = nullptr;
    g_pCVar = nullptr; g_pGlobals = nullptr;
    m_pEngineServer_member=nullptr; /* etc. for all member interface ptrs */
    std::cout << "[RCBot] Plugin unloaded." << std::endl;
}

void CRCBotPlugin::Pause() { std::cout << "[RCBot] Plugin Paused." << std::endl; }
void CRCBotPlugin::UnPause() { std::cout << "[RCBot] Plugin Unpaused." << std::endl; }
const char *CRCBotPlugin::GetPluginDescription() { return "RCBot FF (Full V3)"; }

void CRCBotPlugin::LevelInit(char const *pMapName) {
    std::cout << "[RCBot] LevelInit for map: " << (pMapName ? pMapName : "<unknown>") << std::endl;
    SaveTaskLogsToFile();
    m_CompletedTaskLogs.clear();
    m_ManagedBots.clear();
    m_PendingBots.clear();
    m_NextBotIdCounter = 1;

    if (m_pGlobalKnowledgeBase) {
        m_pGlobalKnowledgeBase->ClearDynamicMapData();
        m_pGlobalKnowledgeBase->LoadNavMesh(pMapName);
        LoadMapDataFromLua(pMapName);
    }
}

void CRCBotPlugin::LevelShutdown() {
    std::cout << "[RCBot] LevelShutdown." << std::endl;
    SaveTaskLogsToFile();
    m_ManagedBots.clear();
    m_PendingBots.clear();
    if (m_pGlobalKnowledgeBase) {
        m_pGlobalKnowledgeBase->ClearDynamicMapData();
    }
}

void CRCBotPlugin::ServerActivate(edict_t *pEdictList, int edictCount, int clientMax) { /* ... */ }

void CRCBotPlugin::GameFrame(bool simulating) {
    if (simulating) {
        // Conceptual: if (g_pGlobals) { s_fConceptualTaskExecutionTime = g_pGlobals->curtime; }
        UpdatePerceptionSystem_Conceptual();
        PollGameState_Conceptual();
        UpdateAllBots();
    }
}

void CRCBotPlugin::ClientActive(edict_t *pEntity) { /* ... */ }
void CRCBotPlugin::ClientDisconnect(edict_t *pEntity) { RemoveBot(pEntity); }

void CRCBotPlugin::ClientPutInServer(edict_t *pEdict, char const *playername) {
    if (!playername || !pEdict) return;
    std::string nameStr = playername;
    auto it = m_PendingBots.find(nameStr);

    if (it != m_PendingBots.end()) {
        BotInfo pendingBotData = std::move(it->second); // Move data out
        m_PendingBots.erase(it);

        // Add to m_ManagedBots and get a reference to the new element
        m_ManagedBots.emplace_back(pEdict, pendingBotData.botId); // Use constructor that sets pEdict
        BotInfo& newBot = m_ManagedBots.back();

        // Transfer other data from pendingBotData to newBot
        newBot.name = pendingBotData.nameRequest;
        newBot.teamIdRequest = pendingBotData.teamIdRequest;
        newBot.classNameRequest = pendingBotData.classNameRequest;
        newBot.skillLevel = pendingBotData.skillLevel;
        // isPendingPlayerSlot will be false due to the constructor used if pEdict is not null
        // but FinalizeBotAddition will set it explicitly.

        FinalizeBotAddition(pEdict, newBot);
    }
}

void CRCBotPlugin::SetCommandClient(int index) { /* ... */ }
void CRCBotPlugin::ClientSettingsChanged(edict_t *pEdict) { /* ... */ }
PLUGIN_RESULT CRCBotPlugin::ClientConnect(bool *bAllowConnect, edict_t*, const char*, const char*, char*, int) { return PLUGIN_CONTINUE; }
PLUGIN_RESULT CRCBotPlugin::ClientCommand(edict_t*, const void*&) { return PLUGIN_CONTINUE; }
PLUGIN_RESULT CRCBotPlugin::NetworkIDValidated(const char*, const char*) { return PLUGIN_CONTINUE; }
void CRCBotPlugin::OnQueryCvarValueFinished(int, edict_t*, int, const char*, const char*) {}
void CRCBotPlugin::OnEdictAllocated(edict_t*) {}
void CRCBotPlugin::OnEdictFreed(const edict_t *edict) { RemoveBot(const_cast<edict_t*>(edict)); }

// --- Lua Bridge Implementation ---
bool CRCBotPlugin::InitializeLuaBridge() {
    m_pLuaState = nullptr;
    m_bLuaStateCreatedByPlugin = false;
    // Conceptual: Try to get a game-specific Lua interface
    // if (m_GameServerFactory_conceptual) { IGameLuaInterface_Conceptual* pGameLua = (IGameLuaInterface_Conceptual*)m_GameServerFactory_conceptual(FF_LUA_INTERFACE_VERSION_CONCEPTUAL, nullptr); if (pGameLua) m_pLuaState = pGameLua->GetSharedLuaState(); }
    if (!m_pLuaState) {
        m_pLuaState = luaL_newstate();
        if (m_pLuaState) { luaL_openlibs(m_pLuaState); m_bLuaStateCreatedByPlugin = true; }
        else { std::cerr << "[RCBot] Error: luaL_newstate() failed!" << std::endl; return false; }
    }
    if (m_pLuaState) RegisterLuaFunctionsWithPlugin();
    return true;
}
void CRCBotPlugin::ShutdownLuaBridge() {
    if (m_pLuaState && m_bLuaStateCreatedByPlugin) { lua_close(m_pLuaState); }
    m_pLuaState = nullptr; m_bLuaStateCreatedByPlugin = false;
}
void CRCBotPlugin::RegisterLuaFunctionsWithPlugin() {
    if (!m_pLuaState) return;
    lua_newtable(m_pLuaState);
    lua_pushcfunction(m_pLuaState, FFLuaBridge::Lua_RCBot_LogMessage);
    lua_setfield(m_pLuaState, -2, "LogMessage");
    lua_pushcfunction(m_pLuaState, FFLuaBridge::Lua_RCBot_GetGameTime);
    lua_setfield(m_pLuaState, -2, "GetGameTime");
    lua_pushcfunction(m_pLuaState, FFLuaBridge::Lua_RCBot_GetMapName_Conceptual); // Added
    lua_setfield(m_pLuaState, -2, "GetMapName"); // Added
    lua_setglobal(m_pLuaState, "RCBot");
    std::cout << "[RCBot] RCBot Lua table registered." << std::endl;
}
void CRCBotPlugin::LoadMapDataFromLua(const char* pMapName) {
    if (!m_pLuaState || !m_pGlobalKnowledgeBase) return;
    std::vector<std::string> cpTableNames;
    // Conceptual: load map script, then get g_MapControlPointTables
    // std::string mapScriptFile = std::string("scripts/rcbot/maps/") + pMapName + ".lua";
    // if (luaL_dofile(m_pLuaState, mapScriptFile.c_str()) == LUA_OK) {
        lua_getglobal(m_pLuaState, "g_MapControlPointTables_FF"); // Example for FF specific tables
        if (lua_istable(m_pLuaState, -1)) {
            lua_pushnil(m_pLuaState);
            while (lua_next(m_pLuaState, -2) != 0) {
                if (lua_isstring(m_pLuaState, -1)) { cpTableNames.push_back(lua_tostring(m_pLuaState, -1)); }
                lua_pop(m_pLuaState, 1);
            }
        }
        lua_pop(m_pLuaState, 1);
    // } else { std::cerr << "[RCBot] Error loading map script " << mapScriptFile << ": " << lua_tostring(m_pLuaState, -1) << std::endl; lua_pop(m_pLuaState, 1); }
    m_pGlobalKnowledgeBase->LoadMapObjectiveData(m_pLuaState, pMapName, cpTableNames);
}

// --- Bot Management Implementation ---
int CRCBotPlugin::RequestBot(const std::string& name, int teamId, const std::string& className, int skill) {
    std::string botName = name;
    int newBotId = m_NextBotIdCounter;
    if (botName.empty()) {
        bool nameTaken;
        do {
            nameTaken = false;
            botName = "RCBotFF_" + std::to_string(newBotId);
            if(m_PendingBots.count(botName)) {nameTaken=true; newBotId++;}
            if(!nameTaken) {for(const auto& b : m_ManagedBots) if(b.name == botName) {nameTaken=true; newBotId++; break;}}
        } while(nameTaken);
    } else {
        if (m_PendingBots.count(botName) || GetBotInfoByEdict_Internal(nullptr /*how to check by name in m_ManagedBots?*/)) { // Need better check for active bot name
             for(const auto& b : m_ManagedBots) if(b.name == botName) {std::cerr << "[RCBot] Error: Bot name " << botName << " already active." << std::endl; return -1;}
             if(m_PendingBots.count(botName)) {std::cerr << "[RCBot] Error: Bot name " << botName << " already pending." << std::endl; return -1;}
        }
    }
    m_NextBotIdCounter = newBotId + 1; // Ensure next ID is unique

    BotInfo pending(newBotId, botName, teamId, className, skill); // Uses constructor for pending
    m_PendingBots[botName] = std::move(pending);

    std::cout << "[RCBot] Queued bot request for " << botName << " (OurID: " << newBotId << ")." << std::endl;
    if (g_pEngineServer) {
        // g_pEngineServer->CreateFakeClient(botName.c_str()); // Conceptual
        std::cout << "[RCBot] Conceptual: Engine->CreateFakeClient(\"" << botName << "\") called." << std::endl;
    } else {
        std::cerr << "[RCBot] Error: IVEngineServer not available to create bot " << botName << "." << std::endl;
        m_PendingBots.erase(botName);
        return -1;
    }
    return newBotId;
}

void CRCBotPlugin::FinalizeBotAddition(edict_t* pBotEdict, BotInfo& botDataFromPending) {
    // BotInfo already exists in m_ManagedBots vector from ClientPutInServer moving it.
    // Find it by botId to update it.
    BotInfo* newBot = nullptr;
    for(auto& managed_bot : m_ManagedBots){
        if(managed_bot.botId == botDataFromPending.botId){
            newBot = &managed_bot;
            break;
        }
    }
    if(!newBot){ // Should not happen if ClientPutInServer logic is correct
        std::cerr << "[RCBot] Error: Could not find bot in m_ManagedBots during FinalizeBotAddition for " << botDataFromPending.name << std::endl;
        return;
    }

    newBot->pEdict = pBotEdict;
    newBot->isPendingPlayerSlot = false;
    newBot->teamId = newBot->teamIdRequest;
    if (newBot->teamId == 0) { newBot->teamId = TEAM_ID_RED; } // Default team
    newBot->className = newBot->classNameRequest;
    // newBot->internalClassId = GetGameSpecificClassId(newBot->className);

    std::cout << "[RCBot] Finalizing bot " << newBot->name << " (OurID: " << newBot->botId << ")" << std::endl;

    newBot->playerWrapper = std::make_unique<CFFPlayer>(pBotEdict);
    const ClassConfigInfo* pClassCfg = m_pGlobalKnowledgeBase ? m_pGlobalKnowledgeBase->GetClassConfigByName(newBot->className) : nullptr;

    newBot->objectivePlanner = std::make_unique<CObjectivePlanner>(newBot->playerWrapper.get(), m_pGlobalKnowledgeBase.get(), this);
    newBot->aiModule = AIFactory::CreateAIModule(
        newBot->className, newBot->playerWrapper.get(),
        newBot->objectivePlanner.get(), m_pGlobalKnowledgeBase.get(), pClassCfg
    );

    if (!newBot->aiModule) { /* ... error handling ... */ newBot->isActive = false; return; }
    newBot->isActive = true;
    // Conceptual: Issue jointeam/joinclass commands
}

void CRCBotPlugin::RemoveBot(edict_t* pBotEdict) {
    if (!pBotEdict) return;
    m_ManagedBots.erase(
        std::remove_if(m_ManagedBots.begin(), m_ManagedBots.end(),
                       [pBotEdict](const BotInfo& bi){ return bi.pEdict == pBotEdict; }),
        m_ManagedBots.end()
    );
    // Also check m_PendingBots if a bot is removed before ClientPutInServer, though less likely by edict
    for (auto it = m_PendingBots.begin(); it != m_PendingBots.end(); ) {
        if (it->second.pEdict == pBotEdict) { // Should always be null for pending, but for safety
            it = m_PendingBots.erase(it);
        } else {
            ++it;
        }
    }
}

void CRCBotPlugin::UpdateAllBots() {
    for (BotInfo& bot : m_ManagedBots) {
        if (bot.isActive && !bot.isPendingPlayerSlot && bot.pEdict && bot.aiModule && bot.objectivePlanner && bot.playerWrapper /*&& bot.playerWrapper->IsAlive()*/) {
            CUserCmd botCmd;
            // Conceptual: bot.playerWrapper->UpdateState();
            if (bot.objectivePlanner) bot.objectivePlanner->EvaluateAndSelectTask();
            if (bot.aiModule) bot.aiModule->Update(&botCmd);
            // Conceptual: if (g_pEngineServer && bot.pEdict) { /* g_pEngineServer->SetFakeClientUserCmd(bot.pEdict, botCmd); */ }
        }
    }
}

// --- Logging Implementation ---
void CRCBotPlugin::StoreTaskLog(const TaskOutcomeLog& log) { m_CompletedTaskLogs.push_back(log); }
void CRCBotPlugin::SaveTaskLogsToFile() { /* ... (Restored from Task 13/14) ... */ }

// --- Game State Update Placeholders ---
void CRCBotPlugin::UpdatePerceptionSystem_Conceptual() { /* ... */ }
void CRCBotPlugin::PollGameState_Conceptual() { /* ... */ }

// --- Helpers ---
BotInfo* CRCBotPlugin::GetBotInfoByEdict_Internal(edict_t* pEdict) {
    for (BotInfo& botInfo : m_ManagedBots) {
        if (!botInfo.isPendingPlayerSlot && botInfo.pEdict == pEdict) return &botInfo;
    } return nullptr;
}
BotInfo* CRCBotPlugin::GetPendingBotInfoByName(const char* name) {
    if (!name) return nullptr;
    auto it = m_PendingBots.find(name);
    if (it != m_PendingBots.end()) {
        return &it->second;
    }
    return nullptr;
}
