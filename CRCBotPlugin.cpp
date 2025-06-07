#include "CRCBotPlugin.h"
#include "FFBaseAI.h"
#include "ObjectivePlanner.h"
#include "FFStateStructs.h"
#include "BotTasks.h"
#include "FFLuaBridge.h"
#include "BotKnowledgeBase.h"
#include "BotLearningData.h"  // For TaskOutcomeLog related operations

extern "C" {
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
}

#include <iostream>
#include <fstream> // For SaveTaskLogsToFile
#include <algorithm>
#include <stdarg.h>
#include <iomanip> // For std::put_time if formatting timestamp

// --- Conceptual Placeholder Types ---
// (Assuming these are defined as in previous steps for CUserCmd, CBaseEntity, CFFPlayer, edict_t)
#ifndef CUSERCMD_CONCEPTUAL_DEF_CRCBOTPLUGIN_CPP
#define CUSERCMD_CONCEPTUAL_DEF_CRCBOTPLUGIN_CPP
struct CUserCmd { int buttons = 0; float forwardmove = 0.0f; float sidemove = 0.0f; Vector viewangles; int weaponselect = 0; };
#endif
#ifndef CBASEENTITY_CONCEPTUAL_DEF_CRCBOTPLUGIN_CPP
#define CBASEENTITY_CONCEPTUAL_DEF_CRCBOTPLUGIN_CPP
class CBaseEntity { public: virtual ~CBaseEntity() {} Vector GetPosition() const { return Vector(0,0,0); } Vector GetWorldSpaceCenter() const { return Vector(0,0,0); } bool IsAlive() const { return true; } int GetTeamNumber() const { return 0; } };
#endif
#ifndef CFFPLAYER_CONCEPTUAL_DEF_CRCBOTPLUGIN_CPP
#define CFFPLAYER_CONCEPTUAL_DEF_CRCBOTPLUGIN_CPP
class CFFPlayer { public: CFFPlayer(edict_t* ed=nullptr) : m_pEdict(ed) {} edict_t* GetEdict() const { return m_pEdict; } bool IsValid() const {return m_pEdict != nullptr;} bool IsAlive() const { return true; } Vector GetPosition() const { return m_CurrentPosition_placeholder; } Vector GetEyePosition() const { return Vector(m_CurrentPosition_placeholder.x, m_CurrentPosition_placeholder.y, m_CurrentPosition_placeholder.z + 64); } Vector GetViewAngles() const { return m_CurrentViewAngles_placeholder; } void SetViewAngles(const Vector& ang) { m_CurrentViewAngles_placeholder = ang; } int GetTeamNumber() const { return 1; } int GetFlags() const { return (1 << 0); } std::string GetNamePlaceholder() const {return "Bot";} ClassConfigInfo* GetClassConfig() {return nullptr;} Vector m_CurrentPosition_placeholder; Vector m_CurrentViewAngles_placeholder; private: edict_t* m_pEdict; };
#endif
// --- End Conceptual Placeholders ---

CRCBotPlugin* g_pRCBotPlugin_Instance = nullptr;

CRCBotPlugin::CRCBotPlugin()
    : m_NextBotIdCounter(1),
      m_pGlobalKnowledgeBase(nullptr),
      m_pLuaState(nullptr)
{
    g_pRCBotPlugin_Instance = this;
    std::cout << "CRCBotPlugin: Constructor." << std::endl;
}

CRCBotPlugin::~CRCBotPlugin() {
    std::cout << "CRCBotPlugin: Destructor." << std::endl;
    // Save any remaining logs just in case, though LevelShutdown/Unload should handle it
    if (!m_CompletedTaskLogs.empty()) {
        SaveTaskLogsToFile();
    }
    m_ManagedBots.clear();
    g_pRCBotPlugin_Instance = nullptr;
}

bool CRCBotPlugin::Load(CreateInterfaceFn interfaceFactory, CreateInterfaceFn gameServerFactory) {
    std::cout << "CRCBotPlugin: Load called." << std::endl;
    m_NextBotIdCounter = 1;

    m_pGlobalKnowledgeBase = std::make_unique<BotKnowledgeBase>();
    if (!m_pGlobalKnowledgeBase) {
        std::cerr << "CRCBotPlugin: Failed to allocate GlobalKnowledgeBase!" << std::endl;
        return false;
    }
    m_pGlobalKnowledgeBase->classConfigs = &m_GlobalClassConfigs;
    m_pGlobalKnowledgeBase->controlPoints = &m_MapControlPoints;
    m_pGlobalKnowledgeBase->payloadPaths = &m_MapPayloadPaths;

    if (!InitializeLuaBridge()) {
        std::cerr << "CRCBotPlugin: Failed to initialize Lua bridge!" << std::endl;
        // Decide if this is fatal. For now, let it load.
    }

    if (m_pLuaState) {
        std::vector<std::string> classCfgTables;
        lua_getglobal(m_pLuaState, "g_ClassConfigTableNames_FF");
        if (lua_istable(m_pLuaState, -1)) {
            lua_pushnil(m_pLuaState);
            while (lua_next(m_pLuaState, -2) != 0) {
                if (lua_isstring(m_pLuaState, -1)) {
                    classCfgTables.push_back(lua_tostring(m_pLuaState, -1));
                }
                lua_pop(m_pLuaState, 1);
            }
        }
        lua_pop(m_pLuaState, 1);
        if (!m_pGlobalKnowledgeBase->LoadGlobalClassConfigs(m_pLuaState, classCfgTables) && classCfgTables.empty()) {
             std::cout << "CRCBotPlugin: No class configs specified by Lua. KB will be empty of class configs." << std::endl;
        }
    } else {
        std::cerr << "CRCBotPlugin: Lua state not available, cannot load class configs from Lua." << std::endl;
    }

    std::cout << "CRCBotPlugin: Loaded successfully." << std::endl;
    return true;
}

void CRCBotPlugin::Unload() {
    std::cout << "CRCBotPlugin: Unload called." << std::endl;
    SaveTaskLogsToFile(); // Save any remaining logs
    LevelShutdown();
    m_ManagedBots.clear();
    m_pGlobalKnowledgeBase.reset();
    ShutdownLuaBridge();
}

// ... other IServerPluginCallbacks stubs (Pause, UnPause, GetPluginDescription) ...
void CRCBotPlugin::Pause() { std::cout << "CRCBotPlugin: Pause." << std::endl; }
void CRCBotPlugin::UnPause() { std::cout << "CRCBotPlugin: UnPause." << std::endl; }
const char *CRCBotPlugin::GetPluginDescription() { return "RCBot FortressForever Plugin V3"; }


void CRCBotPlugin::LevelInit(char const *pMapName) {
    std::cout << "CRCBotPlugin: LevelInit for map: " << (pMapName ? pMapName : "N/A") << std::endl;
    SaveTaskLogsToFile(); // Save logs from previous level
    m_CompletedTaskLogs.clear(); // Clear logs for new level
    m_ManagedBots.clear();
    m_NextBotIdCounter = 1;

    if (m_pGlobalKnowledgeBase) {
        m_pGlobalKnowledgeBase->ClearDynamicMapData();
        m_pGlobalKnowledgeBase->LoadNavMesh(pMapName);
        LoadMapDataFromLua(pMapName);
    } else {
        std::cerr << "CRCBotPlugin: GlobalKnowledgeBase not initialized in LevelInit!" << std::endl;
    }
}

void CRCBotPlugin::LevelShutdown() {
    std::cout << "CRCBotPlugin: LevelShutdown." << std::endl;
    SaveTaskLogsToFile(); // Save logs at end of level
    m_ManagedBots.clear();
    if (m_pGlobalKnowledgeBase) {
        m_pGlobalKnowledgeBase->ClearDynamicMapData();
    }
}

// ... other IServerPluginCallbacks stubs ...
void CRCBotPlugin::ServerActivate(edict_t*, int, int) { std::cout << "CRCBotPlugin: ServerActivate." << std::endl; }
void CRCBotPlugin::GameFrame(bool simulating) { if (simulating) UpdateAllBots(); }
void CRCBotPlugin::ClientActive(edict_t*) { /* ... */ }
void CRCBotPlugin::ClientDisconnect(edict_t *pEntity) { RemoveBot(pEntity); }
void CRCBotPlugin::ClientPutInServer(edict_t *pEntity, char const *playername) {
    std::cout << "CRCBotPlugin: ClientPutInServer - Name: " << playername << std::endl;
    BotInfo* pendingBot = GetPendingBotInfoByName(playername);
    if (pendingBot) {
        // Pass the found BotInfo reference to FinalizeBotAddition
        FinalizeBotAddition(pEntity, *pendingBot);
    }
}
void CRCBotPlugin::SetCommandClient(int) { /* ... */ }
void CRCBotPlugin::ClientSettingsChanged(edict_t*) { /* ... */ }
PLUGIN_RESULT CRCBotPlugin::ClientConnect(bool*, edict_t*, const char*, const char*, char*, int) { return PLUGIN_CONTINUE; }
PLUGIN_RESULT CRCBotPlugin::ClientCommand(edict_t*, const void*&) { return PLUGIN_CONTINUE; }
PLUGIN_RESULT CRCBotPlugin::NetworkIDValidated(const char*, const char*) { return PLUGIN_CONTINUE; }
void CRCBotPlugin::OnQueryCvarValueFinished(int, edict_t*, int, const char*, const char*) { /* ... */ }
void CRCBotPlugin::OnEdictAllocated(edict_t*) { /* ... */ }
void CRCBotPlugin::OnEdictFreed(const edict_t *edict) { RemoveBot(const_cast<edict_t*>(edict)); }


// --- Lua Bridge Implementation ---
bool CRCBotPlugin::InitializeLuaBridge() { /* ... (same as Task 13) ... */
    if (!m_pLuaState) {
        m_pLuaState = luaL_newstate();
        if (m_pLuaState) luaL_openlibs(m_pLuaState);
        else { std::cerr << "CRCBotPlugin ERROR: luaL_newstate() failed!" << std::endl; return false; }
    }
    if (m_pLuaState) RegisterLuaFunctionsWithPlugin();
    return (m_pLuaState != nullptr);
}
void CRCBotPlugin::ShutdownLuaBridge() { /* ... (same as Task 13) ... */
    // if (m_pLuaState /* && m_bOwnsLuaState */) { lua_close(m_pLuaState); }
    m_pLuaState = nullptr;
}
void CRCBotPlugin::RegisterLuaFunctionsWithPlugin() { /* ... (same as Task 13, uses FFLuaBridge) ... */
    if (!m_pLuaState) return;
    lua_newtable(m_pLuaState);
    lua_pushcfunction(m_pLuaState, FFLuaBridge::Lua_RCBot_LogMessage);
    lua_setfield(m_pLuaState, -2, "LogMessage");
    lua_pushcfunction(m_pLuaState, FFLuaBridge::Lua_RCBot_GetGameTime);
    lua_setfield(m_pLuaState, -2, "GetGameTime");
    lua_setglobal(m_pLuaState, "RCBot");
}
void CRCBotPlugin::LoadMapDataFromLua(const char* mapName) { /* ... (same as Task 13) ... */
    if (!m_pLuaState || !m_pGlobalKnowledgeBase) return;
    std::vector<std::string> cpTableNames;
    lua_getglobal(m_pLuaState, "g_MapControlPointTables");
    if (lua_istable(m_pLuaState, -1)) { /* ... populate cpTableNames ... */ }
    lua_pop(m_pLuaState, 1);
    m_pGlobalKnowledgeBase->LoadMapObjectiveData(m_pLuaState, mapName, cpTableNames);
}

// --- Bot Management Implementation ---
BotInfo* CRCBotPlugin::GetPendingBotInfoByName(const char* name) { /* ... (same as Task 13) ... */
    for (BotInfo& botInfo : m_ManagedBots) {
        if (botInfo.isPendingSpawn && botInfo.name == name) return &botInfo;
    } return nullptr;
}
BotInfo* CRCBotPlugin::GetBotInfoByEdict(edict_t* pEdict) { /* ... (same as Task 13) ... */
    for (BotInfo& botInfo : m_ManagedBots) {
        if (!botInfo.isPendingSpawn && botInfo.pEdict == pEdict) return &botInfo;
    } return nullptr;
}

int CRCBotPlugin::RequestBot(const std::string& name, int teamId, const std::string& className, int skill) { /* ... (same as Task 13) ... */
    std::string botName = name;
    if (botName.empty()) botName = "RCBot_" + std::to_string(m_NextBotIdCounter);
    int currentBotId = m_NextBotIdCounter++;
    m_ManagedBots.emplace_back(currentBotId, botName, teamId, className);
    m_ManagedBots.back().skillLevel = skill;
    std::cout << "CRCBotPlugin: Requested bot " << botName << " (ID: " << currentBotId << ")" << std::endl;
    // Conceptual: engine->CreateFakeClient(botName.c_str());
    return currentBotId;
}

// Modified to take BotInfo& directly as found by ClientPutInServer
void CRCBotPlugin::FinalizeBotAddition(edict_t* pBotEdict, BotInfo& botInfo) {
    botInfo.pEdict = pBotEdict;
    botInfo.isPendingSpawn = false;
    // botInfo.internalClassId = ResolveClassNameToGameId(botInfo.classNameRequest);

    std::cout << "CRCBotPlugin: Finalizing bot " << botInfo.name << " with edict " << pBotEdict << std::endl;

    const ClassConfigInfo* selectedClassConfig = nullptr;
    if (m_pGlobalKnowledgeBase && m_pGlobalKnowledgeBase->classConfigs) {
        for (const auto& cfg : *m_pGlobalKnowledgeBase->classConfigs) {
            if (cfg.className == botInfo.classNameRequest) {
                selectedClassConfig = &cfg;
                botInfo.internalClassId = cfg.classId; // Store resolved class ID
                break;
            }
        }
    }
     if (!selectedClassConfig) {
         std::cout << "Warning: ClassConfig for '" << botInfo.classNameRequest << "' not found for bot " << botInfo.name << ". AI may be limited." << std::endl;
    }

    // CFFPlayer* playerWrapper = new CFFPlayer(pBotEdict); // Conceptual, owned by BotInfo if implemented
    botInfo.objectivePlanner = std::make_unique<CObjectivePlanner>(nullptr /*playerWrapper*/, m_pGlobalKnowledgeBase.get(), this /*pluginOwner for logging*/);
    botInfo.aiModule = AIFactory::CreateAIModule(
        botInfo.classNameRequest, nullptr /*playerWrapper*/,
        botInfo.objectivePlanner.get(), m_pGlobalKnowledgeBase.get(), selectedClassConfig
    );

    if (!botInfo.aiModule) {
        std::cout << "CRCBotPlugin: ERROR - Failed to create AI module for " << botInfo.name << std::endl;
        botInfo.isActive = false; return;
    }
    botInfo.isActive = true;
    std::cout << "CRCBotPlugin: Bot " << botInfo.name << " (ID: " << botInfo.botId << ") is now active." << std::endl;
    // Conceptual: EngineClientCommand(pBotEdict, "jointeam %d", botInfo.teamId);
    // Conceptual: EngineClientCommand(pBotEdict, "joinclass %s", botInfo.classNameRequest.c_str());
}

void CRCBotPlugin::RemoveBot(edict_t* pBotEdict) { /* ... (same as Task 13) ... */
    auto it = std::remove_if(m_ManagedBots.begin(), m_ManagedBots.end(),
                             [pBotEdict](const BotInfo& bi){ return !bi.isPendingSpawn && bi.pEdict == pBotEdict; });
    if (it != m_ManagedBots.end()) {
        std::cout << "CRCBotPlugin: Removing bot " << it->name << " (ID: " << it->botId << ")" << std::endl;
        m_ManagedBots.erase(it, m_ManagedBots.end());
    }
}

void CRCBotPlugin::UpdateAllBots() { /* ... (same as Task 13, but ensure CUserCmd is defined) ... */
    for (BotInfo& bot : m_ManagedBots) {
        if (bot.isActive && !bot.isPendingSpawn && bot.aiModule && bot.objectivePlanner && bot.pEdict) {
            CUserCmd botCmd;
            if (bot.objectivePlanner) bot.objectivePlanner->EvaluateAndSelectTask();
            if (bot.aiModule) bot.aiModule->Update(&botCmd);
            // Conceptual: Engine_ApplyUserCmd(bot.pEdict, &botCmd);
        }
    }
}

// --- Logging Implementation ---
void CRCBotPlugin::StoreTaskLog(const TaskOutcomeLog& log) {
    m_CompletedTaskLogs.push_back(log);
    // std::cout << "CRCBotPlugin: Stored task log for bot " << log.botId << " - HLT: " << log.taskDescription << std::endl;
}

void CRCBotPlugin::SaveTaskLogsToFile() {
    if (m_CompletedTaskLogs.empty()) {
        // std::cout << "CRCBotPlugin: No task logs to save." << std::endl;
        return;
    }

    // Conceptual file path, ensure "logs" directory exists or handle creation
    std::string filePath = "logs/rcbot_task_outcomes.jsonl";
    std::ofstream logFile(filePath, std::ios_base::app); // Append mode

    if (!logFile.is_open()) {
        std::cerr << "CRCBotPlugin Error: Could not open task log file: " << filePath << std::endl;
        return;
    }

    std::cout << "CRCBotPlugin: Saving " << m_CompletedTaskLogs.size() << " task logs to " << filePath << "..." << std::endl;

    for (const auto& log : m_CompletedTaskLogs) {
        // Simple JSON-like manual serialization for demonstration
        // A proper JSON library would be better for complex structures.
        logFile << "{";
        logFile << "\"botId\":" << log.botId << ",";
        logFile << "\"botName\":\"" << log.botName << "\","; // Assumes botName is populated in TaskOutcomeLog
        logFile << "\"botClass\":\"" << log.botClassUsed << "\",";
        logFile << "\"taskType\":" << static_cast<int>(log.taskType) << ","; // Log enum as int
        logFile << "\"taskDesc\":\"" << log.taskDescription << "\",";
        logFile << "\"targetNameOrId\":\"" << log.targetNameOrId << "\",";
        logFile << "\"targetPos\":{\"x\":" << log.targetPosition.x << ",\"y\":" << log.targetPosition.y << ",\"z\":" << log.targetPosition.z << "},";

        auto timeToEpochMillis = [](const std::chrono::system_clock::time_point& tp) {
            return std::chrono::duration_cast<std::chrono::milliseconds>(tp.time_since_epoch()).count();
        };
        logFile << "\"startTimeMs\":" << timeToEpochMillis(log.startTime) << ",";
        logFile << "\"endTimeMs\":" << timeToEpochMillis(log.endTime) << ",";
        logFile << "\"durationSec\":" << log.durationSeconds << ",";

        logFile << "\"outcome\":" << static_cast<int>(log.outcome) << ",";
        logFile << "\"outcomeScore\":" << log.outcomeScore << ",";

        logFile << "\"stateAtStart\":{\"allyScore\":" << log.stateAtStart.teamScore_Ally
                << ",\"enemyScore\":" << log.stateAtStart.teamScore_Enemy
                << ",\"objStatus\":\"" << log.stateAtStart.simpleObjectiveStatus << "\"},";
        logFile << "\"stateAtEnd\":{\"allyScore\":" << log.stateAtEnd.teamScore_Ally
                << ",\"enemyScore\":" << log.stateAtEnd.teamScore_Enemy
                << ",\"objStatus\":\"" << log.stateAtEnd.simpleObjectiveStatus << "\"},";

        logFile << "\"subTasks\":[";
        for (size_t i = 0; i < log.executedSubTasks.size(); ++i) {
            const auto& sub = log.executedSubTasks[i];
            logFile << "{\"type\":" << static_cast<int>(sub.type)
                    << ",\"success\":" << (sub.success ? "true" : "false")
                    << ",\"durationSec\":" << sub.durationSeconds
                    << ",\"reason\":\"" << sub.failureReason << "\"}";
            if (i < log.executedSubTasks.size() - 1) logFile << ",";
        }
        logFile << "]";
        logFile << "}\n"; // Newline for JSONL format (one JSON object per line)
    }

    logFile.close();
    std::cout << "CRCBotPlugin: Saved " << m_CompletedTaskLogs.size() << " task logs." << std::endl;
    m_CompletedTaskLogs.clear(); // Clear after saving
}
