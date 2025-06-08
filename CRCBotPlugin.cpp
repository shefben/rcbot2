// --- Start of CRCBotPlugin.cpp ---
#include "CRCBotPlugin.h"

// SDK Headers (ensure paths are correct relative to fortressforever/src/ or your include paths)
#include "public/tier1/convar.h"        // For ConCommand, CCommand, ICvar, FCVAR_ flags
#include "public/engine/ivengineserver.h" // For IVEngineServer
#include "public/igameevents.h"         // For IGameEventManager2, IGameEvent
#include "public/engine/ienginetrace.h"   // For IEngineTrace
#include "public/globalvars_base.h"     // For CGlobalVarsBase (gpGlobals)
#include "public/eiface.h"              // For IServerGameClients, CreateInterfaceFn, etc.
#include "public/edict.h"               // For edict_t utilities like ENTINDEX() if needed
#include "game/shared/shareddefs.h"     // For FF specific TEAM_ and CLASS_ defines (e.g. FF_TEAM_RED, CLASS_SOLDIER)
// #include "game/server/iplayerinfo.h" // Location of IPlayerInfoManager might vary, or use CBasePlayer directly
// #include "engine/ibotmanager.h"   // Location/existence of IBotManager might vary

// Bot Framework Headers
#include "AIFactory.h"
#include "ObjectivePlanner.h"
#include "FFBaseAI.h"
#include "BotKnowledgeBase.h"
#include "FFLuaBridge.h"
#include "BotLearningData.h" // For TaskOutcomeLog
#include "CFFPlayer.h"
#include "FFBot_SDKDefines.h" // Our internal enums like FF_BotPlayerClassID and mapping functions

// Lua headers (ensure your build system links Lua correctly)
extern "C" {
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
}

#include <string>
#include <vector>
#include <memory>
#include <map>
#include <algorithm> // For std::transform
#include <fstream>   // For logging
#include <chrono>    // For logging timestamp

// Global engine interface pointers (initialized in Load, used by various parts of the plugin)
IVEngineServer*       g_pEngineServer = nullptr;
IPlayerInfoManager*   g_pPlayerInfoManager = nullptr;
IServerGameClients* g_pServerGameClients = nullptr;
IBotManager*          g_pBotManager = nullptr; // May remain null if not found/used
IGameEventManager2*   g_pGameEventManager = nullptr;
IEngineTrace*         g_pEngineTraceClient = nullptr;
ICvar*                g_pCVar = nullptr;
CGlobalVarsBase*      g_pGlobals = nullptr; // Should be set to the engine's gpGlobals

// Global plugin instance
CRCBotPlugin g_CRCBotPlugin;
EXPOSE_SINGLE_INTERFACE_GLOBALVAR(CRCBotPlugin, IServerPluginCallbacks, INTERFACEVERSION_ISERVERPLUGINCALLBACKS, g_CRCBotPlugin);

CRCBotPlugin::CRCBotPlugin() :
    m_pLuaState(nullptr), m_bLuaStateCreatedByPlugin(false),
    m_NextBotIdCounter(0), m_bRegisteredForEvents(false),
    m_pEngineServer_member(nullptr), m_pPlayerInfoManager_member(nullptr),
    m_pServerGameClients_member(nullptr), m_pBotManager_member(nullptr),
    m_pGameEventManager_member(nullptr), m_pEngineTraceClient_member(nullptr),
    m_pCVar_member(nullptr), m_pGlobals_member(nullptr),
    m_pCmdBotAdd(nullptr),
    m_fNextFullPerceptionScanTime(0.0f) // Initialize perception scan time
     {
    m_pGlobalKnowledgeBase = std::make_unique<BotKnowledgeBase>();
    // Use Msg/Warning/DevMsg from tier0/dbg.h once interfaces are up, e.g. after Load()
    // printf("[RCBot] Plugin Constructor\n");
}

CRCBotPlugin::~CRCBotPlugin() {
    // printf("[RCBot] Plugin Destructor\n");
}

bool CRCBotPlugin::Load(CreateInterfaceFn interfaceFactory, CreateInterfaceFn gameServerFactory) {
    // Store interface pointers
    g_pEngineServer = (IVEngineServer*)interfaceFactory(INTERFACEVERSION_VENGINESERVER, nullptr);
    g_pGameEventManager = (IGameEventManager2*)interfaceFactory(INTERFACEVERSION_GAMEEVENTSMANAGER2, nullptr);
    g_pEngineTraceClient = (IEngineTrace*)interfaceFactory(INTERFACEVERSION_ENGINETRACE_SERVER, nullptr);
    g_pCVar = (ICvar*)interfaceFactory(CVAR_INTERFACE_VERSION, nullptr); // CVAR_INTERFACE_VERSION from tier1/iconvar.h

    // For CGlobalVarsBase, some SDK versions provide it via IVEngineServer, others it's a global `gpGlobals`
    if (g_pEngineServer) {
         // g_pGlobals = g_pEngineServer->GetGlobals(); // This method might not exist in all SDK versions.
         // If not, assume gpGlobals is available directly from including "globalvars_base.h"
         // and is initialized by the engine.
    }
    if (!g_pGlobals && !gpGlobals) { /* Critical error if neither method works */ return false;}
    if (!g_pGlobals) g_pGlobals = gpGlobals; // Use the direct global if available

    // Game specific interfaces (from gameServerFactory)
    // IPlayerInfoManager might be part of game.dll/tf.dll etc.
    // g_pPlayerInfoManager = (IPlayerInfoManager*)gameServerFactory(INTERFACEVERSION_PLAYERINFOMANAGER, nullptr);
    g_pServerGameClients = (IServerGameClients*)gameServerFactory(INTERFACEVERSION_SERVERGAMECLIENTS, nullptr);

    // IBotManager is often not a standard/public interface. CreateFakeClient is more common.
    // g_pBotManager = (IBotManager*)interfaceFactory("BotManagerInterface001_UsuallyNotFound", nullptr);

    // Assign to member variables
    m_pEngineServer_member = g_pEngineServer;
    m_pPlayerInfoManager_member = g_pPlayerInfoManager; // May be null
    m_pServerGameClients_member = g_pServerGameClients;
    m_pBotManager_member = g_pBotManager; // May be null
    m_pGameEventManager_member = g_pGameEventManager;
    m_pEngineTraceClient_member = g_pEngineTraceClient;
    m_pCVar_member = g_pCVar;
    m_pGlobals_member = g_pGlobals;

    // Check critical interfaces
    if (!m_pEngineServer_member) { /* Log Error */ return false; }
    if (!m_pCVar_member) { /* Log Error */ return false; }
    if (!m_pGlobals_member) { /* Log Error */ return false; }
    if (!m_pGameEventManager_member) { /* Log Warning, events are useful */ }


    if (!InitializeLuaBridge()) { /* Log Error */ }
    if (m_pLuaState && m_pGlobalKnowledgeBase) {
        LoadGlobalClassConfigsFromLua(); // Call new function
    }

    if (m_pGameEventManager_member) {
        m_pGameEventManager_member->AddListener(this, "player_death", false); // false for server-side
        m_pGameEventManager_member->AddListener(this, "player_spawn", false);
        // Add FF specific events: e.g. "ff_event_flag_captured", "ff_event_point_captured" (need actual names)
        // m_pGameEventManager_member->AddListener(this, "ff_flag_captured", false);
        m_bRegisteredForEvents = true;
    }

    if (m_pCVar_member) {
        // In modern Source SDKs, ConCommands register themselves when g_pCVar is set.
        // new ConCommand(...) is enough. Store the pointer to delete it in Unload.
        m_pCmdBotAdd = new ConCommand("rcbot_add", CRCBotPlugin::RCBot_Add_Cmd_Callback,
                                   "Adds an RCBot. Usage: rcbot_add [team] [class] [name] [skill(1-5)]",
                                   FCVAR_PLUGIN_CONCOMMAND_FLAGS);
        // If explicit registration is needed by an older SDK:
        // m_pCVar_member->RegisterConCommand(m_pCmdBotAdd);
    }

    m_NextBotIdCounter = 1; // Start bot IDs from 1
    // Msg("[RCBot] Plugin Loaded Successfully.\n"); // Using SDK's Msg
    return true;
}

void CRCBotPlugin::Unload() {
    // Msg("[RCBot] Plugin Unload.\n");
    SaveTaskLogsToFile();

    if (m_pGameEventManager_member && m_bRegisteredForEvents) {
        m_pGameEventManager_member->RemoveListener(this);
        m_bRegisteredForEvents = false;
    }

    if (m_pCmdBotAdd) { // m_pCVar_member check not strictly needed for delete if ConCommand handles unregistration
        // if (m_pCVar_member) m_pCVar_member->UnregisterConCommand(m_pCmdBotAdd); // If explicit unreg needed
        delete m_pCmdBotAdd;
        m_pCmdBotAdd = nullptr;
    }

    ShutdownLuaBridge();

    m_ManagedBots.clear();
    m_PendingBots.clear();
    m_CompletedTaskLogs.clear();

    // Nullify all interface pointers
    g_pEngineServer = nullptr; g_pPlayerInfoManager = nullptr; g_pServerGameClients = nullptr;
    g_pBotManager = nullptr; g_pGameEventManager = nullptr; g_pEngineTraceClient = nullptr;
    g_pCVar = nullptr; g_pGlobals = nullptr;
    m_pEngineServer_member = nullptr; m_pPlayerInfoManager_member = nullptr; m_pServerGameClients_member = nullptr;
    m_pBotManager_member = nullptr; m_pGameEventManager_member = nullptr; m_pEngineTraceClient_member = nullptr;
    m_pCVar_member = nullptr; m_pGlobals_member = nullptr;
}

void CRCBotPlugin::Pause() { /* Msg("[RCBot] Pause.\n"); */ }
void CRCBotPlugin::UnPause() { /* Msg("[RCBot] UnPause.\n"); */ }
const char *CRCBotPlugin::GetPluginDescription() { return "RCBot FF v1.0"; }

void CRCBotPlugin::LevelInit(char const *pMapName) {
    // Msg("[RCBot] LevelInit: %s\n", pMapName ? pMapName : "N/A");
    SaveTaskLogsToFile();
    m_CompletedTaskLogs.clear();
    m_ManagedBots.clear();
    m_PendingBots.clear();
    m_NextBotIdCounter = 1;
    m_sCurrentMapName = pMapName ? pMapName : "";

    if (m_pGlobalKnowledgeBase) {
        m_pGlobalKnowledgeBase->ClearDynamicMapData();
        if (pMapName) m_pGlobalKnowledgeBase->LoadNavMesh(pMapName); // Assumes .nav file matches map name
        LoadMapDataFromLua(pMapName); // Load map specific Lua data (CPs, etc.)
    }
}
void CRCBotPlugin::LevelShutdown() {
    // Msg("[RCBot] LevelShutdown.\n");
    SaveTaskLogsToFile();
    if (m_pGlobalKnowledgeBase) {
        m_pGlobalKnowledgeBase->ClearDynamicMapData();
    }
    m_ManagedBots.clear();
    m_PendingBots.clear();
}

void CRCBotPlugin::ServerActivate(edict_t *pEdictList, int edictCount, int clientMax) {
    // Msg("[RCBot] ServerActivate. Max Edicts: %d, Max Clients: %d\n", edictCount, clientMax);
}
void CRCBotPlugin::ClientActive(edict_t *pEntity) { /* Not used for now */ }
void CRCBotPlugin::SetCommandClient(int index) { /* Not used for now */ }
void CRCBotPlugin::ClientSettingsChanged(edict_t *pEdict) { /* Not used for now */ }

PLUGIN_RESULT CRCBotPlugin::ClientConnect(bool *bAllowConnect, edict_t *pEntity, const char *pszName, const char *pszAddress, char *reject, int maxrejectlen) {
    // Msg("[RCBot] ClientConnect: %s from %s\n", pszName, pszAddress);
    return PLUGIN_CONTINUE;
}
PLUGIN_RESULT CRCBotPlugin::ClientCommand(edict_t *pEntity, const CCommand &args) {
    // This is for commands typed by actual players, not the server console ConCommands.
    // We could implement player-usable bot commands here if desired.
    return PLUGIN_CONTINUE;
}
PLUGIN_RESULT CRCBotPlugin::NetworkIDValidated(const char *pszUserName, const char *pszNetworkID) { return PLUGIN_CONTINUE; }
void CRCBotPlugin::OnQueryCvarValueFinished(QueryCvarCookie_t iCookie, edict_t *pPlayerEntity, EQueryCvarValueStatus eStatus, const char *pCvarName, const char *pCvarValue) {}
void CRCBotPlugin::OnEdictAllocated(edict_t *edict) { /* Can be used to track new entities if needed */ }
void CRCBotPlugin::OnEdictFreed(const edict_t *edict) {
    // If an edict is freed, and it belongs to one of our bots, remove the bot.
    // The const_cast is typical here as our internal list might not be const.
    RemoveBot(const_cast<edict_t*>(edict));
}

// ConCommand Callback
void CRCBotPlugin::RCBot_Add_Cmd_Callback(const CCommand& args) {
    if (!g_CRCBotPlugin.m_pEngineServer_member) { // Access plugin instance through global
        Warning("[RCBot] RCBot_Add_Cmd_Callback: Engine server not available.\n");
        return;
    }

    std::string teamStr = (args.ArgC() > 1) ? args.Arg(1) : "auto";
    std::string classNameRequestStr = (args.ArgC() > 2) ? args.Arg(2) : "soldier";
    std::string botName = (args.ArgC() > 3) ? args.Arg(3) : "";
    int skillLevel = (args.ArgC() > 4) ? atoi(args.Arg(4)) : 3;

    int teamId = TEAM_ID_BOT_AUTO_ASSIGN;
    std::string lowerTeamStr = teamStr;
    std::transform(lowerTeamStr.begin(), lowerTeamStr.end(), lowerTeamStr.begin(), ::tolower);

    // Using FF_TEAM_ defines from shareddefs.h (assuming they are available)
    if (lowerTeamStr == "red" || teamStr == std::to_string(FF_TEAM_RED)) teamId = FF_TEAM_RED;
    else if (lowerTeamStr == "blue" || teamStr == std::to_string(FF_TEAM_BLUE)) teamId = FF_TEAM_BLUE;
    // Add other FF teams if applicable (e.g., FF_TEAM_GREEN, FF_TEAM_YELLOW)
    else if (lowerTeamStr != "auto" && teamStr != std::to_string(TEAM_ID_BOT_AUTO_ASSIGN)) {
        Msg("[RCBot] Invalid team '%s'. Use 'auto', 'red', 'blue', or numeric team IDs.\n", teamStr.c_str());
        return;
    }

    if (skillLevel < 1) skillLevel = 1;
    if (skillLevel > 5) skillLevel = 5;

    FF_BotPlayerClassID internalClassId = GetBotClassIDFromString_FF(classNameRequestStr);
    if (internalClassId == FF_BOT_CLASS_UNKNOWN) {
        Msg("[RCBot] Invalid class name '%s' requested.\n", classNameRequestStr.c_str());
        return;
    }

    if (g_CRCBotPlugin.RequestBot(botName, teamId, classNameRequestStr, skillLevel)) {
        Msg("[RCBot] Bot request for '%s' (Class: %s, Team: %s, Skill: %d) successful.\n",
            botName.empty() ? classNameRequestStr.c_str() : botName.c_str(),
            classNameRequestStr.c_str(), teamStr.c_str(), skillLevel);
    } else {
        Msg("[RCBot] Bot request for '%s' failed.\n",
            botName.empty() ? classNameRequestStr.c_str() : botName.c_str());
    }
}

bool CRCBotPlugin::RequestBot(const std::string& name, int teamIdReq, const std::string& classNameReq, int skill) {
    if (!m_pEngineServer_member) {
        Warning("[RCBot] Cannot request bot: IVEngineServer not available.\n");
        return false;
    }

    std::string botName = name;
    if (botName.empty()) {
        // Generate a unique name if not provided
        botName = "RCBot_" + classNameReq + "_" + std::to_string(m_NextBotIdCounter);
    }

    // Store pending bot info
    BotInfo pendingBotInfo; // Default constructor
    pendingBotInfo.botId = m_NextBotIdCounter++; // Assign unique ID
    pendingBotInfo.nameRequest = botName;
    pendingBotInfo.isPendingPlayerSlot = true;
    pendingBotInfo.isActive = false;
    pendingBotInfo.teamIdRequest = teamIdReq;
    pendingBotInfo.classNameRequest = classNameReq.empty() ? "soldier" : classNameReq; // Default to soldier
    pendingBotInfo.skillLevel = skill;

    m_PendingBots[botName] = pendingBotInfo; // Add to pending map, keyed by requested name

    // Actual engine call to spawn a bot:
    edict_t* pNewEdict = m_pEngineServer_member->CreateFakeClient(botName.c_str());
    if (pNewEdict) {
        Msg("[RCBot] CreateFakeClient for %s succeeded. Waiting for ClientPutInServer.\n", botName.c_str());
        return true;
    } else {
        Warning("[RCBot] CreateFakeClient for %s failed.\n", botName.c_str());
        m_PendingBots.erase(botName); // Remove from pending if creation failed immediately
        m_NextBotIdCounter--; // Reclaim ID
        return false;
    }
}

void CRCBotPlugin::ClientPutInServer(edict_t *pEdict, char const *playername) {
    if (!playername || pEdict == nullptr || !m_pEngineServer_member) return;

    std::string nameStr = playername;
    auto it = m_PendingBots.find(nameStr);

    if (it != m_PendingBots.end()) {
        BotInfo pendingData = it->second;
        m_PendingBots.erase(it);

        // Create new BotInfo in m_ManagedBots
        m_ManagedBots.emplace_back(pEdict, pendingData.botId); // Use constructor (edict_t*, int)
        BotInfo& newBot = m_ManagedBots.back();

        FinalizeBotAddition(pEdict, newBot, pendingData); // Pass the BotInfo from m_ManagedBots
        Msg("[RCBot] Bot %s finalized and added to managed list.\n", newBot.name.c_str());
    }
}

void CRCBotPlugin::FinalizeBotAddition(edict_t* pBotEdict, BotInfo& newBotInManagedList, const BotInfo& pendingBotData) {
    newBotInManagedList.name = pendingBotData.nameRequest;
    newBotInManagedList.teamIdRequest = pendingBotData.teamIdRequest;
    newBotInManagedList.classNameRequest = pendingBotData.classNameRequest;
    newBotInManagedList.skillLevel = pendingBotData.skillLevel;
    // pEdict and botId are already set by constructor of BotInfo in m_ManagedBots

    newBotInManagedList.isPendingPlayerSlot = false; // No longer pending slot
    newBotInManagedList.isActive = true;

    int finalTeamId = newBotInManagedList.teamIdRequest;
    if (finalTeamId == TEAM_ID_BOT_AUTO_ASSIGN) {
        // Simple auto-team: try to balance. Needs actual player counts per team.
        // int redCount = CountPlayersOnTeam_SDK(FF_TEAM_RED);
        // int blueCount = CountPlayersOnTeam_SDK(FF_TEAM_BLUE);
        // finalTeamId = (redCount <= blueCount) ? FF_TEAM_RED : FF_TEAM_BLUE;
        finalTeamId = FF_TEAM_RED; // Default to RED for now
    }
    newBotInManagedList.teamId = finalTeamId;

    newBotInManagedList.internalClassId = GetBotClassIDFromString_FF(newBotInManagedList.classNameRequest);
    int sdkClassId = GetSDKClassIDFromBotEnum_FF(newBotInManagedList.internalClassId);
    newBotInManagedList.className = GetPlayerClassNameFromBotID_FF(newBotInManagedList.internalClassId);

    if (m_pEngineServer_member && pBotEdict) {
        // Deferring ClientCommand calls to allow the player entity to fully initialize.
        // Might need a short delay or do this in the next GameFrame tick.
        // For now, issue immediately:
        char cmd[128];
        snprintf(cmd, sizeof(cmd), "jointeam %d", newBotInManagedList.teamId); // Use SDK team ID
        m_pEngineServer_member->ClientCommand(pBotEdict, cmd);

        snprintf(cmd, sizeof(cmd), "joinclass %d", sdkClassId); // Use SDK class ID
        m_pEngineServer_member->ClientCommand(pBotEdict, cmd);
    }

    newBotInManagedList.playerWrapper = std::make_unique<CFFPlayer>(pBotEdict); // Pass only edict
    const ClassConfigInfo* pClassCfg = m_pGlobalKnowledgeBase ? m_pGlobalKnowledgeBase->GetClassConfig(newBotInManagedList.className) : nullptr;
    if (!pClassCfg) {
        Warning("[RCBot] Could not get class config for %s. Bot may not behave correctly.\n", newBotInManagedList.className.c_str());
    }

    newBotInManagedList.objectivePlanner = std::make_unique<CObjectivePlanner>(newBotInManagedList.playerWrapper.get(), m_pGlobalKnowledgeBase.get(), this);
    newBotInManagedList.aiModule = AIFactory::CreateAIModule(newBotInManagedList.className, newBotInManagedList.playerWrapper.get(), newBotInManagedList.objectivePlanner.get(), m_pGlobalKnowledgeBase.get(), pClassCfg);

    if (!newBotInManagedList.aiModule) {
        Warning("[RCBot] Failed to create AI module for bot %s (Class: %s).\n", newBotInManagedList.name.c_str(), newBotInManagedList.className.c_str());
        newBotInManagedList.isActive = false;
    }
}

void CRCBotPlugin::RemoveBot(edict_t* pBotEdict) {
    if (!pBotEdict) return;
    for (auto it = m_ManagedBots.begin(); it != m_ManagedBots.end(); ++it) {
        if (it->pEdict == pBotEdict) {
            Msg("[RCBot] Removing bot: %s (ID: %d)\n", it->name.c_str(), it->botId);
            // Any specific AI module cleanup if needed before erase
            it->aiModule.reset();
            it->objectivePlanner.reset();
            it->playerWrapper.reset();
            m_ManagedBots.erase(it);
            return;
        }
    }
    // Also check pending bots if an edict was assigned then freed before ClientPutInServer (unlikely but possible)
    for (auto it = m_PendingBots.begin(); it != m_PendingBots.end(); ++it) {
        if (it->second.pEdict == pBotEdict) { // Though pEdict for pending is usually null until ClientPutInServer
            Msg("[RCBot] Removing pending bot: %s\n", it->second.nameRequest.c_str());
            m_PendingBots.erase(it);
            return;
        }
    }
}

void CRCBotPlugin::ClientDisconnect(edict_t *pEntity) {
    RemoveBot(pEntity);
}

void CRCBotPlugin::GameFrame(bool simulating) {
    if (!simulating || !m_pGlobalKnowledgeBase || !m_pGlobals_member ) {
        return;
    }
    // All per-frame logic happens here
    // float currentTime = m_pGlobals_member->curtime;
    // float frameTime = m_pGlobals_member->frametime;

    UpdatePerceptionSystem();
    PollGameState();
    UpdateAllBots(); // This will generate CUserCmds for bots
}

void CRCBotPlugin::UpdateAllBots() {
    if (!m_pEngineServer_member) return;

    for (BotInfo& bot : m_ManagedBots) {
        if (bot.isActive && !bot.isPendingPlayerSlot && bot.pEdict &&
            bot.aiModule && bot.objectivePlanner && bot.playerWrapper) {

            if (!bot.playerWrapper->IsAlive()) continue; // Skip dead bots

            CUserCmd currentCmd; // SDK's CUserCmd, typically from game/shared/usercmd.h
            // Initialize CUserCmd if necessary (e.g. command_number, tick_count from player's last cmd or new prediction)
            // currentCmd.command_number = bot.playerWrapper->GetCurrentUserCommandNumber_Conceptual() + 1;
            // currentCmd.tick_count = m_pGlobals_member->tickcount; // Or predicted tick

            bot.objectivePlanner->EvaluateAndSelectTask();
            bot.aiModule->Update(&currentCmd); // AI fills the CUserCmd

            // Apply the command to the bot. The exact mechanism depends on the game/engine version.
            // For many Source 1 games, there isn't a direct SetFakeClientUserCmd.
            // BotRunCommand in IServerGameDLL is one way, or specific game hooks.
            // If CreateFakeClient makes them behave like real clients, the engine might handle it.
            // For now, this is a conceptual placeholder for applying the bot's CUserCmd.
            // m_pEngineServer_member->SetFakeClientUserCmd_Conceptual(bot.pEdict, currentCmd);
            // OR if IServerGameDLL is available:
            // if (g_pServerGameDLL) g_pServerGameDLL->BotRunCommand(bot.pEdict, &currentCmd);
        }
    }
}

bool CRCBotPlugin::InitializeLuaBridge() {
    m_pLuaState = nullptr;
    m_bLuaStateCreatedByPlugin = false;

    // Conceptual: Attempt to get a game-shared Lua state if available
    // m_pLuaState = SomeGameInterface->GetLuaState();

    if (!m_pLuaState) {
        m_pLuaState = luaL_newstate();
        if (m_pLuaState) {
            luaL_openlibs(m_pLuaState);
            m_bLuaStateCreatedByPlugin = true;
            // Msg("[RCBot] Created new Lua state.\n");
        } else {
            Warning("[RCBot] Failed to create Lua state.\n");
            return false;
        }
    } else {
        // Msg("[RCBot] Using existing/shared Lua state.\n");
    }
    RegisterLuaFunctionsWithPlugin();
    return true;
}

void CRCBotPlugin::ShutdownLuaBridge() {
    if (m_pLuaState && m_bLuaStateCreatedByPlugin) {
        lua_close(m_pLuaState);
        // Msg("[RCBot] Closed plugin-created Lua state.\n");
    }
    m_pLuaState = nullptr;
    m_bLuaStateCreatedByPlugin = false;
}

void CRCBotPlugin::RegisterLuaFunctionsWithPlugin() {
    if (!m_pLuaState) return;
    // Assumes FFLuaBridge functions are compatible with SDK lua_State
    lua_newtable(m_pLuaState);
    lua_pushcfunction(m_pLuaState, FFLuaBridge::Lua_RCBot_LogMessage); // Ensure this is static or global
    lua_setfield(m_pLuaState, -2, "LogMessage");
    lua_pushcfunction(m_pLuaState, FFLuaBridge::Lua_RCBot_GetGameTime); // Ensure this is static or global
    lua_setfield(m_pLuaState, -2, "GetGameTime");
    // Add more functions as needed
    lua_setglobal(m_pLuaState, "RCBot"); // Create global table "RCBot"
}

bool CRCBotPlugin::LoadGlobalClassConfigsFromLua() {
    if (!m_pLuaState || !m_pGlobalKnowledgeBase) return false;
    return m_pGlobalKnowledgeBase->LoadGlobalClassConfigs(m_pLuaState); // Removed table names, now handled in KB
}

void CRCBotPlugin::LoadMapDataFromLua(const char* pMapName) {
    if (m_pLuaState && m_pGlobalKnowledgeBase && pMapName) {
        m_pGlobalKnowledgeBase->LoadMapObjectiveData(m_pLuaState, pMapName); // Removed table names
    }
}

void CRCBotPlugin::StoreTaskLog(const TaskOutcomeLog& log) {
    m_CompletedTaskLogs.push_back(log);
}
void CRCBotPlugin::SaveTaskLogsToFile() {
    if (m_CompletedTaskLogs.empty() || m_sCurrentMapName.empty()) return;
    // std::string filename = "rcbot_task_log_" + m_sCurrentMapName + "_" + std::to_string(std::time(nullptr)) + ".jsonl";
    // std::ofstream outFile(filename, std::ios_base::app);
    // if (!outFile.is_open()) { return; }
    // for (const auto& log : m_CompletedTaskLogs) { /* ... JSONL writing ... */ }
    // outFile.close();
    m_CompletedTaskLogs.clear();
}

void CRCBotPlugin::UpdatePerceptionSystem() {
    // if (!m_pEngineServer_member || !m_pGlobals_member || !m_pGlobalKnowledgeBase || !m_pEngineTraceClient_member) return;

    // float currentTime = m_pGlobals_member->curtime;
    // if (currentTime < m_fNextFullPerceptionScanTime) return; // Throttle full scan
    // m_fNextFullPerceptionScanTime = currentTime + FULL_PERCEPTION_SCAN_INTERVAL_FF;

    std::vector<TrackedEntityInfo> perceivedPlayers;
    std::vector<BuildingInfo> perceivedBuildings;
    std::vector<ReflectableProjectileInfo> perceivedProjectiles;

    // --- Iterate Players ---
    // for (int i = 1; i <= m_pGlobals_member->maxClients; ++i) {
    //     edict_t* pEdict = m_pEngineServer_member->PEntityOfEntIndex(i);
    //     if (!pEdict || m_pEngineServer_member->IsEdictFree(pEdict)) continue;

    //     CBaseEntity* pBaseEnt = CBaseEntity::Instance(pEdict); // SDK: Get CBaseEntity from edict
    //     if (!pBaseEnt || !pBaseEnt->IsPlayer() || !pBaseEnt->IsAlive()) continue;

    //     // Skip self if this perception system is run for each bot individually and has context of self.
    //     // If it's a global system, this check might not be needed or be different.
    //     // if (pEdict == bot_owner_edict) continue;

    //     ::CFFPlayer* pSDKPlayer = static_cast<::CFFPlayer*>(pBaseEnt);
    //     TrackedEntityInfo pInfo(pEdict, i); // Use edict index as entityId for now

    //     pInfo.lastKnownPosition = pSDKPlayer->GetAbsOrigin();
    //     pInfo.velocity = pSDKPlayer->GetAbsVelocity();
    //     pInfo.health = pSDKPlayer->GetHealth();
    //     pInfo.maxHealth = pSDKPlayer->GetMaxHealth(); // Or pSDKPlayer->m_PlayerClass.GetClassData()->m_iHealth
    //     pInfo.team = pSDKPlayer->GetTeamNumber();
    //     pInfo.className = pSDKPlayer->m_PlayerClass.GetClassData()->m_szClassName; // Actual class name string from SDK

    //     pInfo.isOnFire = (pSDKPlayer->GetFlags() & FL_ONFIRE); // Example, FF might use a condition like m_nPlayerCondEx
    //     // pInfo.fireExpireTime = pInfo.isOnFire ? (currentTime + pSDKPlayer->GetRemainingFireDuration_Conceptual()) : 0.0f;

    //     if (pSDKPlayer->m_PlayerClass.GetClassNum() == CLASS_SPY) { // Use actual SDK CLASS_SPY
    //         pInfo.isDisguised_conceptual = pSDKPlayer->IsDisguised();
    //         if (pInfo.isDisguised_conceptual) {
    //             pInfo.displayedTeam_conceptual = pSDKPlayer->GetDisguiseTeam();
    //             pInfo.displayedClassName_conceptual = GetPlayerClassNameFromBotID_FF(GetBotClassIDFromSlot_FF(pSDKPlayer->GetDisguiseClass()));
    //         }
    //     }

    //     // Basic Visibility (Placeholder - real LOS is complex and per-bot)
    //     pInfo.isVisible = true;
    //     pInfo.lastSeenTime = pInfo.isVisible ? currentTime : 0.f; // Update if visible, else needs old data

    //     perceivedPlayers.push_back(pInfo);
    // }

    // --- Iterate Other Entities (Buildings, Projectiles) ---
    // This loop should go up to m_pGlobals_member->maxEntities or a known higher edict limit.
    // for (int i = m_pGlobals_member->maxClients + 1; i < MAX_EDICTS_FROM_SOMEWHERE; ++i) {
    //     edict_t* pEdict = m_pEngineServer_member->PEntityOfEntIndex(i);
    //     if (!pEdict || m_pEngineServer_member->IsEdictFree(pEdict)) continue;
    //     CBaseEntity* pBaseEnt = CBaseEntity::Instance(pEdict);
    //     if (!pBaseEnt /* || !pBaseEnt->IsNetworkable() - some projectiles might not be alive */) continue;

    //     const char* entityClassName = pBaseEnt->GetClassname(); // SDK: Get CBaseEntity classname

    //     // Building Processing
    //     if (IsBuildingClassName_Conceptual(entityClassName)) { // Use helper with actual FF classnames
    //         BuildingInfo buildingInfo;
    //         buildingInfo.pEdict = pEdict;
    //         buildingInfo.uniqueId = i;
    //         buildingInfo.type = GetBuildingTypeFromClassName_Conceptual(entityClassName); // Use helper
    //         buildingInfo.position = pBaseEnt->GetAbsOrigin();
    //         buildingInfo.teamId = pBaseEnt->GetTeamNumber();
    //         buildingInfo.health = pBaseEnt->GetHealth();
    //         // buildingInfo.maxHealth = pBaseEnt->GetMaxHealth(); // Or GetBuildingMaxHealth(type, level)
    //         // buildingInfo.level = pBaseEnt->GetBuildingLevel_Conceptual();
    //         // buildingInfo.isSapped = pBaseEnt->IsSapped_Conceptual();
    //         // buildingInfo.isBuildingInProgress = pBaseEnt->IsBuilding_Conceptual();
    //         // buildingInfo.builderPlayerId = pBaseEnt->GetBuilderUserID_Conceptual(); // If available
    //         perceivedBuildings.push_back(buildingInfo);
    //     }
    //     // Projectile Processing
    //     else if (IsReflectableProjectileClassName_Conceptual(entityClassName)) { // Use helper
    //         // int myBotsTeam = TEAM_ID_NONE; // Get the team of the bot(s) this perception update is for
    //         // if (!m_ManagedBots.empty() && m_ManagedBots[0].playerWrapper) myBotsTeam = m_ManagedBots[0].playerWrapper->GetTeam();

    //         // if (pBaseEnt->GetTeamNumber() != myBotsTeam && pBaseEnt->GetTeamNumber() != TEAM_UNASSIGNED) {
    //         //     ReflectableProjectileInfo projInfo;
    //         //     projInfo.pEntity = pEdict;
    //         //     projInfo.projectileId = i;
    //         //     projInfo.position = pBaseEnt->GetAbsOrigin();
    //         //     projInfo.velocity = pBaseEnt->GetAbsVelocity();
    //         //     projInfo.isHostile = true;
    //         //     // projInfo.projectileType = GetProjectileTypeFromClassName_Conceptual(entityClassName);
    //         //     // projInfo.creationTime = pBaseEnt->GetCreationTime_Conceptual();
    //         //     perceivedProjectiles.push_back(projInfo);
    //         // }
    //     }
    // } // End entity iteration

    if (m_pGlobalKnowledgeBase) {
        m_pGlobalKnowledgeBase->UpdateTrackedPlayers(perceivedPlayers); // Use renamed method
        m_pGlobalKnowledgeBase->UpdateTrackedBuildings(perceivedBuildings); // Use renamed method
        m_pGlobalKnowledgeBase->UpdateTrackedProjectiles(perceivedProjectiles);
    }
}

void CRCBotPlugin::PollGameState() {
    // if (!m_pGlobalKnowledgeBase || !m_pGlobals_member) return;
    // This function is for game states that are not event-driven or need very frequent checks.
    // For example, iterating through known control points and querying their state directly from entities.

    // for (const auto& cp_config_info : m_pGlobalKnowledgeBase->GetControlPointsConfig_Conceptual()) {
    //     // CBaseEntity* cpEnt = FindEntityByTargetname_Conceptual(cp_config_info.targetname); // Or by index if known
    //     // if (cpEnt) {
    //     //     int currentOwner = cpEnt->GetNetProp_Conceptual<int>("m_iTeamOwningCapPoint", TEAM_UNASSIGNED);
    //     //     float captureProgress = cpEnt->GetNetProp_Conceptual<float>("m_flCaptureProgress", 0.0f);
    //     //     bool isLocked = cpEnt->GetNetProp_Conceptual<bool>("m_bCPLocked", false);
    //     //     m_pGlobalKnowledgeBase->UpdateControlPointState(cp_config_info.id, currentOwner, captureProgress, isLocked);
    //     // }
    // }
}

void CRCBotPlugin::FireGameEvent(IGameEvent *event) {
    if (!event || !m_pGlobalKnowledgeBase) return;
    const char *eventName = event->GetName();
    if (!eventName) return;

    // Use Q_strcmp from SDK's tier1/strtools.h (or cstring.h) for case-sensitive comparison.
    // Or V_stricmp for case-insensitive if event names might vary by case.
    // Assuming FF_EVENT_POINT_CAPTURED, etc. are const char* defined in FFBot_SDKDefines.h

    // if (Q_strcmp(eventName, FF_EVENT_POINT_CAPTURED) == 0) {
    //     int cpID = event->GetInt("cp_index"); // Use actual key name from FF event
    //     int newTeam = event->GetInt("team_owning"); // Use actual key name
    //     // float progress = event->GetFloat("capture_progress", 0.0f); // If available
    //     // bool locked = event->GetBool("cp_locked", false); // If available
    //     m_pGlobalKnowledgeBase->UpdateControlPointState(cpID, newTeam, 1.0f /*assume full cap on event*/, false);
    // }
    // else if (Q_strcmp(eventName, FF_EVENT_PLAYER_DEATH) == 0) {
    //     int victimUserID = event->GetInt("userid");       // Victim's UserID
    //     int attackerUserID = event->GetInt("attacker");   // Attacker's UserID
    //     // Potentially update BotKnowledgeBase with kill/death info, or notify AI modules.
    // }
    // else if (Q_strcmp(eventName, FF_EVENT_PLAYER_SPAWN) == 0) {
    //     int spawnUserID = event->GetInt("userid");
    //     // A player (bot or human) has spawned. Perception system will pick them up,
    //     // but this event can be used for immediate actions or state resets.
    // }
    // else if (Q_strcmp(eventName, FF_EVENT_BUILDING_PLACED) == 0) {
    //     // BuildingInfo info;
    //     // info.builderPlayerId = event->GetInt("builder_userid");
    //     // info.teamId = event->GetInt("teamid");
    //     // info.type = static_cast<BuildingType_FF>(event->GetInt("building_type_id")); // Assuming type is an int ID
    //     // info.position.x = event->GetFloat("pos_x"); /* ... y, z ... */
    //     // info.pEdict = FindEdictForNewlyPlacedBuilding_Conceptual(info); // This is tricky, event might not give edict
    //     // if (info.pEdict || info.uniqueId != -1 /* if event provides a unique ID for the building */) {
    //     //     m_pGlobalKnowledgeBase->UpdateOrAddBuilding(info);
    //     // }
    // }
    // else if (Q_strcmp(eventName, FF_EVENT_BUILDING_DESTROYED) == 0) {
    //     // int buildingOwnerUserId = event->GetInt("builder_userid");
    //     // int buildingUniqueId = event->GetInt("building_unique_id"); // If available
    //     // edict_t* pBuildingEdict = FindEdictFromEventData_Conceptual(event); // If available
    //     // if (pBuildingEdict) m_pGlobalKnowledgeBase->RemoveBuilding(pBuildingEdict);
    //     // else if (buildingUniqueId != -1) m_pGlobalKnowledgeBase->RemoveBuildingByUniqueId_Conceptual(buildingUniqueId);
    // }
    // Add more FF-specific events and their handling.
}

// --- End of CRCBotPlugin.cpp ---
