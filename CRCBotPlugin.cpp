#include "CRCBotPlugin.h"
#include "FFStateStructs.h"      // Also includes BotDefines.h implicitly if structured that way
#include "BotKnowledgeBase.h"
#include "FFLuaBridge.h"
#include "ObjectivePlanner.h"
#include "AIFactory.h"
#include "CFFPlayer.h"
#include "BotDefines.h" // For TEAM_ID constants etc.

#include <iostream> // For std::cout, std::cerr (conceptual logging, to be removed or replaced)
#include <fstream>  // For file I/O (task logging)
#include <algorithm> // For std::transform
#include <chrono>    // For logging timestamps

// --- Conceptual Global Engine Interface Pointers ---
// These would normally be initialized in `Load()` by querying the engine.
// For conceptual purposes, some might be defined here if not passed around.
// IVEngineServer* g_pEngineServer_conceptual = nullptr; // Prefer member m_pEngineServer_member
// ICvar* g_pCVar_conceptual = nullptr; // Prefer member m_pCVar_member

// --- Global Plugin Instance ---
// The EXPOSE macro handles the creation of the instance.
// We need a global pointer or reference to it for the static ConCommand callback.
CRCBotPlugin g_CRCBotPlugin; // Definition of the global instance

// --- Static ConCommand for adding bots ---
// Initialized in Load()
static ConCommand_Placeholder s_cmdBotAdd;

// --- EXPOSE_SINGLE_INTERFACE_GLOBALVAR Macro (Conceptual Definition) ---
// This macro connects the global instance (g_CRCBotPlugin) to the engine's plugin system.
// It should match the definition in GameDefines_Placeholder.h or be defined here if not included.
// (Assuming it's in GameDefines_Placeholder.h, so not redefining here)
EXPOSE_SINGLE_INTERFACE_GLOBALVAR_IMPLEMENTED(CRCBotPlugin, "IServerPluginCallbacks", g_CRCBotPlugin, INTERFACEVERSION_ISERVERPLUGINCALLBACKS_CURRENT);


// --- Helper Functions (Conceptual Logging) ---
// void ConMsg(const char* format, ...) {
//     // Conceptual: actual engine ConMsg
//     // va_list args;
//     // va_start(args, format);
//     // vprintf(format, args); // Simple stdout print for concept
//     // va_end(args);
//     // printf("\n");
// }


// --- CRCBotPlugin Implementation ---
CRCBotPlugin::CRCBotPlugin() :
    m_pKnowledgeBase(std::make_unique<BotKnowledgeBase>()),
    m_pLuaState_member(nullptr),
    m_bLuaInitialized(false),
    m_bPluginLoaded(false),
    m_CommandClientIndex(-1) {
    // ConMsg("CRCBotPlugin: Constructor called.");
}

CRCBotPlugin::~CRCBotPlugin() {
    // ConMsg("CRCBotPlugin: Destructor called.");
    // Ensure cleanup, though Unload should handle most of it.
    if (m_bLuaInitialized) {
        ShutdownLuaBridge();
    }
}

bool CRCBotPlugin::Load(void* (*interfaceFactory)(const char *pName, int *pReturnCode),
                        void* (*gameServerFactory)(const char *pName, int *pReturnCode)) {
    // ConMsg("RCBot: Load called.");

    // Get Engine Interfaces (Conceptual)
    // int returnCode = 0;
    // m_pEngineServer_member = static_cast<IVEngineServer*>(interfaceFactory(VENGINESERVER_INTERFACE_VERSION_CURRENT, &returnCode));
    // if (!m_pEngineServer_member) { /* ConMsg("RCBot Error: Failed to get IVEngineServer."); */ return false; }

    // m_pPlayerInfoManager_member = static_cast<IPlayerInfoManager_Conceptual*>(interfaceFactory(INTERFACEVERSION_PLAYERINFOMANAGER_CURRENT, &returnCode));
    // if (!m_pPlayerInfoManager_member) { /* ConMsg("RCBot Error: Failed to get IPlayerInfoManager."); */ return false; }

    // m_pServerGameClients_member = static_cast<IServerGameClients_Conceptual*>(interfaceFactory("ServerGameClients004", &returnCode)); // Example version
    // if (!m_pServerGameClients_member) { /* ConMsg("RCBot Error: Failed to get IServerGameClients."); */ return false; }

    // m_pGameEventManager_member = static_cast<IGameEventManager2_Conceptual*>(interfaceFactory(INTERFACEVERSION_GAMEEVENTSMANAGER2_CURRENT, &returnCode));
    // if (!m_pGameEventManager_member) { /* ConMsg("RCBot Error: Failed to get IGameEventManager2."); */ return false; }
    // else { /* m_pGameEventManager_member->AddListener(this, "player_death", true); */ /* Conceptual */ }

    // m_pCVar_member = static_cast<ICvar_Conceptual*>(interfaceFactory(CVAR_INTERFACE_VERSION_CURRENT, &returnCode));
    // if (!m_pCVar_member) { /* ConMsg("RCBot Warning: Failed to get ICvar. Bot commands may not be available."); */ }
    // else {
        // Initialize and Register ConCommand
        s_cmdBotAdd = ConCommand_Placeholder("rcbot_add", CRCBotPlugin::RCBot_Add_Cmd_Callback,
                                         "Adds an RCBot. Usage: rcbot_add [team] [class] [name] [skill(1-5)]",
                                         FCVAR_GAMEDLL_CONCEPTUAL | FCVAR_CHEAT_CONCEPTUAL | FCVAR_SERVER_CAN_EXECUTE_CONCEPTUAL);
        // m_pCVar_member->RegisterConCommand(&s_cmdBotAdd); // Conceptual registration
        s_cmdBotAdd.m_bRegistered_conceptual = true; // Manually mark as "registered" for our placeholder
        // ConMsg("RCBot: Conceptually registered 'rcbot_add' console command.");
    // }

    // m_pBotManager_member = static_cast<IBotManager_Conceptual*>(interfaceFactory("PlayerBotManager001_Engine", &returnCode)); // Example version
    // if (!m_pBotManager_member) { /* ConMsg("RCBot Warning: Failed to get IBotManager. Engine's CreateBot might not be usable."); */ }

    // m_pEngineTrace_member = static_cast<IEngineTrace_Conceptual*>(interfaceFactory("EngineTraceServer004", &returnCode)); // Example
    // if (!m_pEngineTrace_member) { /* ConMsg("RCBot Warning: Failed to get IEngineTrace."); */ }


    if (!InitializeLuaBridge()) {
        // ConMsg("RCBot Error: Failed to initialize Lua bridge in Load().");
        // Depending on design, this might be fatal or just disable Lua features.
    }

    if (!LoadGlobalClassConfigsFromLua()) {
        // ConMsg("RCBot Warning: Could not load global class configs from Lua in Load(). Bots may not behave correctly.");
    }

    // ConMsg("RCBot: Plugin Loaded Successfully.");
    m_bPluginLoaded = true;
    return true;
}

void CRCBotPlugin::Unload(void) {
    // ConMsg("RCBot: Unload called.");

    // Unregister ConCommand
    // if (m_pCVar_member && s_cmdBotAdd.IsRegistered_Conceptual()) {
        // m_pCVar_member->UnregisterConCommand(&s_cmdBotAdd); // Conceptual unregistration
        s_cmdBotAdd.m_bRegistered_conceptual = false;
        // ConMsg("RCBot: Conceptually unregistered 'rcbot_add' console command.");
    // }

    // Remove all bots
    while (!m_ManagedBots.empty()) {
        // This is a bit rough, assumes bot name or index can be derived or we use a generic removal
        // If bots have specific edicts, use that. For now, just clear.
        // Conceptual: Kick all bots from the game
        // if (m_ManagedBots.back()->pEdict && m_pEngineServer_member) {
        //     char cmd[64];
        //     snprintf(cmd, sizeof(cmd), "kickid %d", m_ManagedBots.back()->userId_conceptual); // Assuming userId is known
        //     m_pEngineServer_member->ServerCommand(cmd);
        // }
        m_ManagedBots.pop_back();
    }
    m_PendingBots.clear();
    // ConMsg("RCBot: All bots removed/cleared.");

    SaveTaskLogsToFile(); // Save any remaining logs

    ShutdownLuaBridge();

    // Release engine interfaces (conceptual, smart pointers would handle this)
    // m_pEngineServer_member = nullptr;
    // m_pPlayerInfoManager_member = nullptr;
    // m_pServerGameClients_member = nullptr;
    // m_pGameEventManager_member = nullptr; // remove listener if AddListener was called
    // m_pCVar_member = nullptr;
    // m_pBotManager_member = nullptr;
    // m_pEngineTrace_member = nullptr;

    m_bPluginLoaded = false;
    // ConMsg("RCBot: Plugin Unloaded.");
}

void CRCBotPlugin::Pause(void) { /* ConMsg("RCBot: Pause called."); */ }
void CRCBotPlugin::UnPause(void) { /* ConMsg("RCBot: UnPause called."); */ }
const char* CRCBotPlugin::GetPluginDescription(void) { return "RCBot - A Rule-Based Bot for Fortress Forever"; }

void CRCBotPlugin::LevelInit(char const *pMapName) {
    // ConMsg("RCBot: LevelInit for map %s.", pMapName ? pMapName : "unknown");
    m_CurrentMapName_Log = pMapName ? pMapName : "unknown_map";
    m_CompletedTaskLogs.clear(); // Clear logs for the new map

    if (!m_bLuaInitialized) { // If Lua wasn't set up in Load or was shut down
        if (!InitializeLuaBridge()) {
            // ConMsg("RCBot Error: Failed to initialize Lua bridge in LevelInit.");
            return; // Critical failure for map-specific data
        }
    }
    if (!LoadGlobalClassConfigsFromLua()){ // Reload in case of changes or if Load failed this part
         // ConMsg("RCBot Warning: Could not load global class configs from Lua in LevelInit.");
    }

    if (m_pKnowledgeBase) {
        m_pKnowledgeBase->ClearMapSpecificData(); // Clear old nav mesh, objectives
        if (!LoadMapDataFromLua(m_CurrentMapName_Log)) {
            // ConMsg("RCBot Warning: Failed to load Lua map data for %s. Bots may not navigate or understand objectives.", m_CurrentMapName_Log.c_str());
            // Attempt to load a default or generate placeholder nav data if possible
            // m_pKnowledgeBase->LoadNavMesh("placeholder_nav_mesh.nav"); // Conceptual
        }
    } else {
        // ConMsg("RCBot Error: KnowledgeBase is null in LevelInit.");
    }
}

void CRCBotPlugin::ServerActivate(edict_t *pEdictList, int edictCount, int clientMax) {
    // ConMsg("RCBot: ServerActivate called. Max Edicts: %d, Max Clients: %d", edictCount, clientMax);
    // World is fully up, game rules active. Bots requested before this might need final setup.
    // Or iterate pEdictList to find existing players/entities to populate KnowledgeBase.
}

void CRCBotPlugin::GameFrame(bool simulating) {
    if (!simulating || !m_bPluginLoaded) {
        return;
    }

    // Conceptual: Update game time in KnowledgeBase or via a global accessor
    // float currentTime = m_pEngineServer_member ? m_pEngineServer_member->GetTime() : 0.0f;
    // if (m_pKnowledgeBase) m_pKnowledgeBase->SetCurrentTime(currentTime);


    // UpdatePerceptionSystem_Conceptual(); // Discover/update enemies, allies, projectiles
    // PollGameState_Conceptual();          // Update control points, payload, flags

    // Max usercmds we can generate: For now, assume a capacity equal to managed bots.
    // In reality, the engine provides a CUserCmd array to fill.
    // For conceptual logic, we'll pass a dummy array or handle it inside UpdateAllBots.
    if (!m_ManagedBots.empty()) {
        // CUserCmd conceptual_usercmds[MAX_BOTS_CONCEPTUAL]; // MAX_BOTS_CONCEPTUAL would be a define
        // UpdateAllBots(conceptual_usercmds, MAX_BOTS_CONCEPTUAL);
        // for (size_t i = 0; i < m_ManagedBots.size(); ++i) {
        //     if (m_ManagedBots[i] && m_ManagedBots[i]->isActive && m_ManagedBots[i]->pEdict) {
        //         // Conceptual: m_pEngineServer_member->SetBotCommand(m_ManagedBots[i]->pEdict, conceptual_usercmds[i]);
        //     }
        // }
        UpdateAllBots(nullptr, 0); // Pass nullptr if CUserCmd is handled internally by CFFPlayer for concept
    }
}

void CRCBotPlugin::LevelShutdown(void) {
    // ConMsg("RCBot: LevelShutdown called.");
    SaveTaskLogsToFile();
    m_CompletedTaskLogs.clear();

    // Remove all bots without trying to kick (server is shutting down)
    for (auto& bot : m_ManagedBots) {
        if (bot) {
            // ConMsg("RCBot: Clearing bot %s during level shutdown.", bot->name.c_str());
        }
    }
    m_ManagedBots.clear();
    m_PendingBots.clear();
    // ConMsg("RCBot: All bots cleared from management lists.");

    if (m_pKnowledgeBase) {
        m_pKnowledgeBase->ClearMapSpecificData();
    }

    // Optionally shut down Lua here if it's per-map, but Load/Unload handles plugin-lifetime Lua.
    // ShutdownLuaBridge(); // This might be too aggressive if Lua holds cross-level global configs.
    // ConMsg("RCBot: Level shutdown complete.");
}

void CRCBotPlugin::ClientActive(edict_t *pEntity) { /* May use to track human players */ }
void CRCBotPlugin::ClientDisconnect(edict_t *pEntity) { /* May use to track human players */ }

void CRCBotPlugin::ClientPutInServer(edict_t *pEntity, char const *playername) {
    // ConMsg("RCBot: ClientPutInServer: %s", playername);
    // Check if this playername matches any in m_PendingBots
    auto it = m_PendingBots.find(std::string(playername));
    if (it != m_PendingBots.end()) {
        // ConMsg("RCBot: Matched pending bot: %s", playername);
        FinalizeBotAddition(*(it->second), pEntity); // Pass the BotInfo unique_ptr's content
        m_ManagedBots.push_back(std::move(it->second)); // Transfer ownership to m_ManagedBots
        m_PendingBots.erase(it); // Remove from pending
    }
}

void CRCBotPlugin::SetCommandClient(int index) { m_CommandClientIndex = index; }
void CRCBotPlugin::ClientSettingsChanged(edict_t *pEdict) { /* For player settings changes */ }

int CRCBotPlugin::ClientConnect(bool *bAllowConnect, edict_t *pEntity, const char *pszName,
                                const char *pszAddress, char *reject, int maxrejectlen) {
    // ConMsg("RCBot: ClientConnect: %s from %s", pszName, pszAddress);
    *bAllowConnect = true; // Default: allow connection
    return 0;
}

void CRCBotPlugin::ClientCommand(edict_t *pEntity, const CCommand &args) {
    // This is for client commands executed on the server by players.
    // The ConCommand callback (RCBot_Add_Cmd_Callback) is for server console commands.
    // const char* cmd = args.Arg(0);
    // if (pEntity && cmd) {
        // ConMsg("RCBot: ClientCommand from entity %p: %s", pEntity, cmd);
        // Example: if (stricmp(cmd, "bot_add_from_client") == 0) { /* process */ }
    // }
}

void CRCBotPlugin::NetworkIDValidated(const char *pszUserName, const char *pszNetworkID) {}
void CRCBotPlugin::OnQueryCvarValueFinished(int iCookie, edict_t *pPlayerEntity, int eStatus,
                                           const char *pCvarName, const char *pCvarValue) {}
void CRCBotPlugin::OnEdictAllocated(edict_t *edict) {}
void CRCBotPlugin::OnEdictFreed(edict_t *edict) {
    // If an edict being freed belongs to one of our managed bots, clean up the bot.
    for (auto it = m_ManagedBots.begin(); it != m_ManagedBots.end(); ++it) {
        if ((*it) && (*it)->pEdict == edict) {
            // ConMsg("RCBot: Edict for bot %s is being freed. Removing bot.", (*it)->name.c_str());
            // Perform necessary cleanup, similar to RemoveBot but without kicking.
            (*it)->isActive = false;
            // Log or handle any specific shutdown tasks for this bot
            m_ManagedBots.erase(it); // Remove from managed list
            break;
        }
    }
}


void CRCBotPlugin::FireGameEvent(void *event_conceptual /* IGameEvent *event */) {
    // if (!m_pGameEventManager_member || !event_conceptual) return;
    // const char *eventName = // m_pGameEventManager_member->GetEventName(event_conceptual); // Conceptual
    // if (strcmp(eventName, "player_death") == 0) {
        // int victimUserID = // event_conceptual->GetInt("userid");
        // int attackerUserID = // event_conceptual->GetInt("attacker");
        // ConMsg("RCBot: Player death event: Victim %d, Attacker %d", victimUserID, attackerUserID);
        // Update bot knowledge, check if a bot was involved, etc.
        // for (auto& bot : m_ManagedBots) {
        //     if (bot && bot->pPlayer && bot->pPlayer->GetUserID_Conceptual() == victimUserID) {
        //         if (bot->pPlanner) bot->pPlanner->OnBotKilled(bot->pPlayer.get());
        //     }
        // }
    // }
    // else if (strcmp(eventName, "teamplay_point_captured") == 0) { // FF specific event name
        // int cpID = // event_conceptual->GetInt("cp");
        // int newOwnerTeam = // event_conceptual->GetInt("team");
        // if (m_pKnowledgeBase) m_pKnowledgeBase->UpdateControlPointOwner_Conceptual(cpID, newOwnerTeam);
    // }
}

bool CRCBotPlugin::RequestBot(const std::string& name_in, int teamId, const std::string& className_in, int skill) {
    // ConMsg("RCBot: RequestBot: Name '%s', Team %d, Class '%s', Skill %d", name_in.c_str(), teamId, className_in.c_str(), skill);
    std::string botName = name_in;
    if (botName.empty()) {
        // Generate a unique name, e.g., "RCBot_Soldier_1"
        // For now, just use class name
        botName = "RCBot_" + className_in;
        // Add a number to make it unique if multiple unnamed bots of same class
        int count = 0;
        for(const auto& bot : m_ManagedBots) if(bot && bot->name.rfind(botName, 0) == 0) count++;
        for(const auto& pair : m_PendingBots) if(pair.second && pair.second->name.rfind(botName, 0) == 0) count++;
        if (count > 0) botName += "_" + std::to_string(count);

    }

    // Validate class (conceptual, relies on BotKnowledgeBase class configs)
    // if (m_pKnowledgeBase && !m_pKnowledgeBase->GetClassConfigByName(className_in)) {
        // ConMsg("RCBot Error: Invalid class '%s' requested for bot '%s'.", className_in.c_str(), botName.c_str());
        // return false;
    // }

    auto newBotInfo = std::make_unique<BotInfo>(botName, teamId, className_in, skill);

    // Conceptual: Call engine's bot creation function
    // edict_t* botEdict = m_pBotManager_member ? m_pBotManager_member->CreateBot(botName.c_str()) : nullptr;
    // if (botEdict) {
    //     FinalizeBotAddition(*newBotInfo, botEdict);
    //     m_ManagedBots.push_back(std::move(newBotInfo));
    //     ConMsg("RCBot: Engine created bot %s directly.", botName.c_str());
    // } else {
        // If engine doesn't create it immediately or we need manual spawn:
        // This assumes the game will eventually call ClientPutInServer for a bot spawned via "sv_addbot" or similar
        // if (m_pEngineServer_member) {
        //     char cmd[256];
        //     snprintf(cmd, sizeof(cmd), "sv_addbot %s %d %s %d", // Conceptual command
        //              botName.c_str(), teamId, className_in.c_str(), skill);
        //     m_pEngineServer_member->ServerCommand(cmd);
            m_PendingBots[botName] = std::move(newBotInfo); // Store in pending, wait for ClientPutInServer
        //     ConMsg("RCBot: Bot %s added to pending list. Issued conceptual sv_addbot command.", botName.c_str());
        // } else {
        //     ConMsg("RCBot Error: Cannot create bot. No engine server or bot manager.");
        //     return false;
        // }
    // }
    return true;
}

void CRCBotPlugin::FinalizeBotAddition(BotInfo& botInfo, edict_t* pEdict) {
    // ConMsg("RCBot: Finalizing addition of bot %s.", botInfo.name.c_str());
    botInfo.pEdict = pEdict;
    botInfo.isActive = true;
    // botInfo.userId = m_pPlayerInfoManager_member ? m_pPlayerInfoManager_member->GetPlayerUserID(pEdict) : -1; // Conceptual

    // Set team and class using engine commands if necessary
    // if (m_pEngineServer_member) {
    //     char cmd[128];
    //     snprintf(cmd, sizeof(cmd), "jointeam_conceptual %d %d", botInfo.userId, botInfo.requestedTeamId);
    //     m_pEngineServer_member->ServerCommand(cmd);
    //     snprintf(cmd, sizeof(cmd), "joinclass_conceptual %d %s", botInfo.userId, botInfo.requestedClassName.c_str());
    //     m_pEngineServer_member->ServerCommand(cmd);
    // }

    botInfo.pPlayer = std::make_unique<CFFPlayer>(pEdict, this);
    // const ClassConfigInfo* classConfig = m_pKnowledgeBase ? m_pKnowledgeBase->GetClassConfigByName(botInfo.requestedClassName) : nullptr;
    // if (!classConfig) {
    //     ConMsg("RCBot Error: Could not get class config for %s during FinalizeBotAddition.", botInfo.requestedClassName.c_str());
    //     botInfo.isActive = false; return; // Critical failure for this bot
    // }

    botInfo.pPlanner = std::make_unique<CObjectivePlanner>(botInfo.pPlayer.get(), m_pKnowledgeBase.get(), this /*plugin_ref*/);
    // botInfo.pAIModule = AIFactory::CreateAIModule(classConfig->BotAIClassName, botInfo.pPlayer.get(), botInfo.pPlanner.get(), m_pKnowledgeBase.get(), classConfig);

    // if (!botInfo.pAIModule) {
    //     ConMsg("RCBot Error: Failed to create AI module for bot %s (class %s).", botInfo.name.c_str(), botInfo.requestedClassName.c_str());
    //     botInfo.isActive = false; // Cannot function without AI
    // } else {
    //     ConMsg("RCBot: Bot %s (Class: %s, Team: %d) finalized and active.", botInfo.name.c_str(), botInfo.requestedClassName.c_str(), botInfo.requestedTeamId);
    // }
}


void CRCBotPlugin::RCBot_Add_Cmd_Callback(const CCommand& args) {
    // This is a static function. It calls RequestBot on the global g_CRCBotPlugin instance.
    // if (!g_CRCBotPlugin.m_pEngineServer_member) { // Accessing member of global instance
    //     // ConMsg("RCBot_Add_Cmd Error: Engine server not available.");
    //     return;
    // }

    // Arg(0) is "rcbot_add"
    std::string teamStr     = (args.ArgC() > 1) ? args.Arg(1) : "auto";
    std::string className   = (args.ArgC() > 2) ? args.Arg(2) : "soldier"; // Default class
    std::string botName     = (args.ArgC() > 3) ? args.Arg(3) : "";
    int skillLevel          = (args.ArgC() > 4) ? std::atoi(args.Arg(4)) : 3;

    int teamId = TEAM_ID_AUTO_FF_CONCEPTUAL;
    std::string lowerTeamStr = teamStr;
    std::transform(lowerTeamStr.begin(), lowerTeamStr.end(), lowerTeamStr.begin(),
                   [](unsigned char c){ return std::tolower(c); });

    if (lowerTeamStr == "red" || lowerTeamStr == "2") {
        teamId = TEAM_ID_RED_FF_CONCEPTUAL;
    } else if (lowerTeamStr == "blue" || lowerTeamStr == "3") {
        teamId = TEAM_ID_BLUE_FF_CONCEPTUAL;
    } else if (lowerTeamStr != "auto") {
        // g_CRCBotPlugin.GetEngineServer()->ConMsg("RCBot: Invalid team '%s'. Use 'auto', 'red', or 'blue'.\n", teamStr.c_str());
        return;
    }

    if (skillLevel < 1) skillLevel = 1;
    if (skillLevel > 5) skillLevel = 5;

    // Conceptual: Validate class name (e.g., against BotKnowledgeBase class configs)
    // if (g_CRCBotPlugin.GetKnowledgeBase() && !g_CRCBotPlugin.GetKnowledgeBase()->GetClassConfigByName(className)) {
    //    g_CRCBotPlugin.GetEngineServer()->ConMsg("RCBot: Invalid class '%s' requested.\n", className.c_str());
    //    return;
    // }

    if (g_CRCBotPlugin.RequestBot(botName, teamId, className, skillLevel)) {
        // g_CRCBotPlugin.GetEngineServer()->ConMsg("RCBot: Bot request for '%s' (Class: %s, Team: %s, Skill: %d) successful.\n",
        //                                       botName.empty() ? "Unnamed" : botName.c_str(), className.c_str(), teamStr.c_str(), skillLevel);
    } else {
        // g_CRCBotPlugin.GetEngineServer()->ConMsg("RCBot: Bot request for '%s' failed.\n",
        //                                       botName.empty() ? "Unnamed" : botName.c_str());
    }
}


void CRCBotPlugin::RemoveBot(const std::string& nameOrIndex) {
    // ConMsg("RCBot: Attempting to remove bot: %s", nameOrIndex.c_str());
    // Iterate m_ManagedBots, find by name or edict index, then remove.
    // Conceptual: if (m_pEngineServer_member) { m_pEngineServer_member->ServerCommand("kick " + nameOrIndex); }
    // The OnEdictFreed callback should handle removal from m_ManagedBots if kick is successful.
}

void CRCBotPlugin::UpdateAllBots(CUserCmd* usercmds_output_array, int max_usercmds_capacity) {
    for (size_t i = 0; i < m_ManagedBots.size(); ++i) {
        BotInfo* bot = m_ManagedBots[i].get();
        if (bot && bot->isActive && bot->pPlayer && bot->pAIModule && bot->pPlanner) {
            // Conceptual: CUserCmd for this bot. If usercmds_output_array is null, CFFPlayer might manage its own.
            CUserCmd currentCmd;
            // if (usercmds_output_array && i < (size_t)max_usercmds_capacity) {
            //    currentCmd = &usercmds_output_array[i];
            // } else if (usercmds_output_array) {
            //    // ConMsg("RCBot Warning: Exceeded usercmd capacity in UpdateAllBots.");
            //    continue;
            // }

            bot->pPlanner->EvaluateAndSelectTask(); // Planner decides on HighLevelTask
            bot->pAIModule->Update(&currentCmd);    // AI executes SubTasks from Planner and fills CUserCmd

            // Conceptual: If not using usercmds_output_array, CFFPlayer might queue the command.
            // if (!usercmds_output_array && bot->pPlayer) {
            //    bot->pPlayer->ExecuteUserCmd_Conceptual(currentCmd);
            // }
        }
    }
}

bool CRCBotPlugin::InitializeLuaBridge() {
    // ConMsg("RCBot: Initializing Lua bridge...");
    if (m_bLuaInitialized && m_pLuaState_member) {
        // ConMsg("RCBot: Lua bridge already initialized.");
        return true;
    }
    // m_pLuaState_member = FFLuaBridge::CreateLuaState_Conceptual(); // Or GetGameLuaState_Conceptual();
    // if (!m_pLuaState_member) {
    //     ConMsg("RCBot Error: Could not create or get Lua state.");
    //     return false;
    // }
    // FFLuaBridge::OpenLuaLibraries_Conceptual(m_pLuaState_member);
    // RegisterLuaFunctionsWithPlugin(); // Register RCBot specific functions
    m_bLuaInitialized = true;
    // ConMsg("RCBot: Lua bridge initialized successfully.");
    return true;
}

void CRCBotPlugin::ShutdownLuaBridge() {
    // ConMsg("RCBot: Shutting down Lua bridge...");
    // if (m_pLuaState_member && m_bLuaInitialized) {
    //     FFLuaBridge::CloseLuaState_Conceptual(m_pLuaState_member);
    //     m_pLuaState_member = nullptr;
    //     m_bLuaInitialized = false;
    //     ConMsg("RCBot: Lua bridge shut down.");
    // } else {
    //     ConMsg("RCBot: Lua bridge was not active or already shut down.");
    // }
}

void CRCBotPlugin::RegisterLuaFunctionsWithPlugin() {
    // if (!m_pLuaState_member || !m_bLuaInitialized) {
    //     ConMsg("RCBot Error: Cannot register Lua functions, bridge not initialized.");
    //     return;
    // }
    // ConMsg("RCBot: Registering plugin-specific Lua functions...");
    // Example:
    // FFLuaBridge::RegisterFunction_Conceptual(m_pLuaState_member, "RCBot_GetTime", Lua_RCBot_GetGameTime_Conceptual_Callback);
    // FFLuaBridge::RegisterFunction_Conceptual(m_pLuaState_member, "RCBot_GetMapName", Lua_RCBot_GetMapName_Conceptual_Callback);
    // FFLuaBridge::RegisterUserData_Conceptual(m_pLuaState_member, "g_PluginKnowledgeBase", m_pKnowledgeBase.get()); // Risky, manage lifetime
}

bool CRCBotPlugin::LoadMapDataFromLua(const std::string& mapName) {
    // if (!m_pLuaState_member || !m_bLuaInitialized || !m_pKnowledgeBase) {
    //     ConMsg("RCBot Error: Cannot load map data from Lua. Bridge or KB not ready.");
    //     return false;
    // }
    // std::string scriptPath = "lua/maps/" + mapName + ".lua"; // Conceptual path
    // ConMsg("RCBot: Attempting to load map data from Lua script: %s", scriptPath.c_str());
    // if (FFLuaBridge::ExecuteLuaFile_Conceptual(m_pLuaState_member, scriptPath.c_str())) {
        // FFLuaBridge::CallLuaFunction_Conceptual(m_pLuaState_member, "LoadMapData", m_pKnowledgeBase.get()); // Pass KB to Lua
        // ConMsg("RCBot: Successfully executed Lua map script and called LoadMapData for %s.", mapName.c_str());
        // m_pKnowledgeBase->LoadNavMesh(mapName + ".nav"); // Conceptual: also load binary nav mesh
        // return true;
    // } else {
    //     ConMsg("RCBot Error: Failed to execute Lua map script %s.", scriptPath.c_str());
    //     return false;
    // }
    return true; // Placeholder if Lua not fully active
}

bool CRCBotPlugin::LoadGlobalClassConfigsFromLua() {
    // if (!m_pLuaState_member || !m_bLuaInitialized || !m_pKnowledgeBase) {
    //     ConMsg("RCBot Error: Cannot load class configs from Lua. Bridge or KB not ready.");
    //     return false;
    // }
    // std::string scriptPath = "lua/global/class_configs.lua"; // Conceptual path
    // ConMsg("RCBot: Attempting to load global class configs from Lua script: %s", scriptPath.c_str());
    // if (FFLuaBridge::ExecuteLuaFile_Conceptual(m_pLuaState_member, scriptPath.c_str())) {
        // FFLuaBridge::CallLuaFunction_Conceptual(m_pLuaState_member, "LoadAllClassConfigs", m_pKnowledgeBase.get());
        // ConMsg("RCBot: Successfully loaded global class configs from Lua.");
        // return true;
    // } else {
    //     ConMsg("RCBot Error: Failed to execute Lua script for global class configs: %s.", scriptPath.c_str());
    //     return false;
    // }
    return true; // Placeholder if Lua not fully active
}


void CRCBotPlugin::StoreTaskLog(const TaskOutcomeLog& log) {
    m_CompletedTaskLogs.push_back(log);
}

void CRCBotPlugin::SaveTaskLogsToFile() {
    // if (m_CompletedTaskLogs.empty()) return;

    // std::string filename = "rcbot_task_log_" + m_CurrentMapName_Log + "_" + std::to_string(std::time(0)) + ".jsonl";
    // std::ofstream outFile(filename, std::ios_base::app);
    // if (!outFile.is_open()) {
    //     ConMsg("RCBot Error: Could not open task log file %s for writing.", filename.c_str());
    //     return;
    // }
    // ConMsg("RCBot: Saving %zu task logs to %s...", m_CompletedTaskLogs.size(), filename.c_str());
    // for (const auto& log : m_CompletedTaskLogs) {
    //     // Simple JSON-like structure, not using a full JSON lib for conceptual simplicity
    //     outFile << "{";
    //     outFile << "\"task_name\":\"" << log.taskName << "\",";
    //     outFile << "\"task_id\":" << log.taskNumericID << ",";
    //     outFile << "\"bot_name\":\"" << log.botName << "\",";
    //     outFile << "\"bot_class\":\"" << log.botClass << "\",";
    //     outFile << "\"start_time_epoch\":" << log.startTimeEpoch << ",";
    //     outFile << "\"end_time_epoch\":" << log.endTimeEpoch << ",";
    //     outFile << "\"duration_ms\":" << log.durationMs << ",";
    //     outFile << "\"outcome\":\"" << TaskOutcomeToString(log.outcome) << "\",";
    //     outFile << "\"details\":\"" << log.details << "\"";
    //     outFile << "}\n";
    // }
    // outFile.close();
    // m_CompletedTaskLogs.clear(); // Clear after saving
}

// Placeholder for actual perception updates
void CRCBotPlugin::UpdatePerceptionSystem_Conceptual() {
    // For each bot:
    //   - Scan for nearby entities (players, projectiles, objectives)
    //   - Update BotKnowledgeBase with TrackedEntityInfo
    //   - Handle visibility checks, FOV, etc.
}

// Placeholder for direct game state polling (if events are not sufficient)
void CRCBotPlugin::PollGameState_Conceptual() {
    // - Update Control Point status from game state
    // - Update Payload status
    // - Update Flag status for CTF
    // This data would then refresh the BotKnowledgeBase
}
