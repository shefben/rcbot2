#include "CRCBotPlugin.h"
#include "ff_lua_bridge.h" // Contains Populate... functions if still separate
#include "FFBaseAI.h"       // For CFFBaseAI and BotKnowledgeBase
#include "ObjectivePlanner.h" // For CObjectivePlanner

// Lua headers
extern "C" {
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
}

#include <iostream>
#include <stdarg.h> // For va_list in EngineIssueBotCommand
#include <algorithm> // For std::remove_if

// Define the global plugin pointer
CRCBotPlugin* g_pRCBotPlugin = nullptr;

// Conceptual engine interface pointers (replace with actual SDK includes and interface getters)
IVEngineServer* engine = nullptr; // Assume this gets initialized in Load()
CGlobalVarsBase* gpGlobals = nullptr; // Assume this gets initialized in Load()
// IServerGameClients* servergameclients = nullptr; // For iterating players

// --- Constructor / Destructor ---
CRCBotPlugin::CRCBotPlugin() :
    m_pLuaState(nullptr),
    m_bLuaBridgeInitialized(false),
    m_pEngineServer(nullptr),
    m_pServerGameClients(nullptr),
    m_pGlobals(nullptr),
    m_CurrentMapName(""),
    m_NextBotUniqueId(1)
{
    g_pRCBotPlugin = this;
    m_pKnowledgeBase = std::make_unique<BotKnowledgeBase>(); // Initialize KB
}

CRCBotPlugin::~CRCBotPlugin() {
    ShutdownLuaBridge();
    // AI modules and planners in m_ManagedBots will be auto-deleted by unique_ptr
    g_pRCBotPlugin = nullptr;
}

// --- IServerPluginCallbacks (Conceptual Implementations) ---
bool CRCBotPlugin::Load(CreateInterfaceFn interfaceFactory, CreateInterfaceFn gameServerFactory) {
    std::cout << "CRCBotPlugin::Load - Plugin Loaded." << std::endl;
    if (interfaceFactory) {
        // Conceptual: these strings are placeholders for actual interface version strings
        m_pEngineServer = (IVEngineServer*)interfaceFactory("VEngineServer022", nullptr); // Example
        m_pServerGameClients = (IServerGameClients*)interfaceFactory("ServerGameClients004", nullptr); // Example
        // gpGlobals is often accessed via a global pointer set up by the engine, or through an interface
        // For now, direct use of a conceptual gpGlobals pointer.
    }
    if (!m_pEngineServer) {
        std::cerr << "CRITICAL ERROR: Could not get IVEngineServer interface!" << std::endl;
        // return false; // In a real plugin, this would be a load failure
    }
    // engine = m_pEngineServer; // If using the global `engine` pointer

    return true;
}

void CRCBotPlugin::Unload() {
    std::cout << "CRCBotPlugin::Unload - Plugin Unloaded." << std::endl;
}

void CRCBotPlugin::LevelInit(char const *pMapName) {
    m_CurrentMapName = pMapName ? pMapName : "";
    std::cout << "CRCBotPlugin::LevelInit - Map: " << m_CurrentMapName << std::endl;

    m_ManagedBots.clear(); // Clear bots from previous map
    m_PendingBots.clear();

    // Initialize Lua bridge, load map scripts, and populate data structures
    if (!InitializeLuaBridge(m_CurrentMapName)) {
        LogMessageToConsole("ERROR: Failed to initialize Lua bridge.");
    }
    // Initialize BotKnowledgeBase with map specific data (navmesh etc.)
    // m_pKnowledgeBase->navMesh = ... load navmesh ...
    // m_pKnowledgeBase->controlPoints = &m_ControlPoints;
    // m_pKnowledgeBase->payloadPaths = &m_PayloadPaths;
}

void CRCBotPlugin::ServerActivate(edict_t *pEdictList, int edictCount, int clientMax) {
    std::cout << "CRCBotPlugin::ServerActivate." << std::endl;
}

void CRCBotPlugin::GameFrame(bool simulating) {
    if (simulating && m_bLuaBridgeInitialized) { // Only run if Lua is up
        PluginCycle();
    }
}

void CRCBotPlugin::LevelShutdown() {
    std::cout << "CRCBotPlugin::LevelShutdown." << std::endl;
    ShutdownLuaBridge();
    m_ControlPoints.clear();
    m_PayloadPaths.clear();
    m_ClassConfigs.clear();
    m_ManagedBots.clear();
    m_PendingBots.clear();
    m_CurrentMapName = "";
    if (m_pKnowledgeBase) { // Clear KB pointers
        m_pKnowledgeBase->navMesh = nullptr;
        m_pKnowledgeBase->controlPoints = nullptr;
        m_pKnowledgeBase->payloadPaths = nullptr;
    }
}

void CRCBotPlugin::ClientPutInServer(edict_t *pEntity, char const *playername) {
    // Check if this entity is one of our pending bots
    for (size_t i = 0; i < m_PendingBots.size(); ++i) {
        // Matching by name is fragile; a better way would be if the engine call to add a bot
        // returned a user ID or if we could set a specific var on the bot before it's put in server.
        // For now, name is the primary link.
        if (m_PendingBots[i].name == playername) {
            BotInfo infoToFinalize = m_PendingBots[i];
            m_PendingBots.erase(m_PendingBots.begin() + i);
            FinalizeBotSetup(pEntity, infoToFinalize);
            return;
        }
    }
}

void CRCBotPlugin::ClientDisconnect(edict_t* pEntity) {
    OnBotLeaveGame(pEntity);
}


// --- Bot Creation/Management ---
bool CRCBotPlugin::RequestNewBot(const char* desiredTeamStr, const char* desiredClassStr, const char* botNameStr, int skill) {
    BotInfo pendingBot;
    pendingBot.isPendingSpawn = true;
    pendingBot.skillLevel = skill;

    // 1. Determine Bot Name
    if (botNameStr && botNameStr[0] != '\0') {
        pendingBot.name = botNameStr;
    } else {
        char generatedName[64];
        snprintf(generatedName, sizeof(generatedName), "RCBot_%d", m_NextBotUniqueId++);
        pendingBot.name = generatedName;
    }

    // 2. Determine Team
    std::string teamCmd = "spectator"; // Default to spectator if logic fails
    if (desiredTeamStr && (strcmp(desiredTeamStr, "auto") == 0 || desiredTeamStr[0] == '\0')) {
        int redCount = GetTeamPlayerCount(2);  // Assuming TF2-like: 2 for RED
        int blueCount = GetTeamPlayerCount(3); // Assuming TF2-like: 3 for BLUE
        pendingBot.assignedTeamId = (redCount <= blueCount) ? 2 : 3;
    } else if (desiredTeamStr && (strcmp(desiredTeamStr, "red") == 0 || strcmp(desiredTeamStr, "2") == 0)) {
        pendingBot.assignedTeamId = 2;
    } else if (desiredTeamStr && (strcmp(desiredTeamStr, "blue") == 0 || strcmp(desiredTeamStr, "3") == 0)) {
        pendingBot.assignedTeamId = 3;
    } else {
        LogMessageToConsole("Warning: Invalid team specified for bot. Defaulting to auto/spectator.");
        pendingBot.assignedTeamId = (GetTeamPlayerCount(2) <= GetTeamPlayerCount(3)) ? 2 : 3; // Fallback to auto
    }
    teamCmd = (pendingBot.assignedTeamId == 2) ? "red" : "blue";


    // 3. Determine Class
    pendingBot.className = (desiredClassStr && desiredClassStr[0] != '\0') ? desiredClassStr : "scout"; // Default to scout
    // In a real game, map className to a classId (e.g., TF_CLASS_SCOUT)
    // pendingBot.desiredClassId = GetClassIdFromName(pendingBot.className.c_str());
    // For now, we'll use the string.
    pendingBot.desiredClassId = 1; // Placeholder ID

    // Conceptual: Tell the engine to create a bot entity.
    // This is highly engine-specific. It might involve calling a global function,
    // or using a command like "bot_add_fake_client" or similar if the engine supports it.
    // For this example, we'll assume there's a way to make the engine create a generic bot entity
    // that we then take control of. If the engine itself has a "create_bot" command that
    // allows specifying name, team, class, that's even better.
    // engine->CreateFakeClient(pendingBot.name.c_str()); // This is purely conceptual

    // Store in pending list. FinalizeBotSetup will be called from ClientPutInServer.
    m_PendingBots.push_back(pendingBot);
    LogMessageToConsole(("Bot requested: " + pendingBot.name + " Team: " + teamCmd + " Class: " + pendingBot.className).c_str());

    // The following commands would ideally be run *after* the engine confirms the bot entity exists.
    // For some engines, bot_add might handle team/class. If not:
    // EngineIssueBotCommand(nullptr, "bot_add_named %s", pendingBot.name.c_str()); // This is if engine has such a cmd
    // Then, ClientPutInServer would find this named bot.
    // If not, we wait for ClientPutInServer for *any* new bot and try to match by name.

    // For testing the structure, we can simulate the command part.
    // In a real scenario, these are run on an *existing* edict.
    // For now, let's assume an engine command adds the bot and then we configure it.
    // If there's no direct engine command to add a bot that the plugin can control,
    // this part is more complex and might involve finding a newly connected generic entity.

    // Conceptual: Trigger engine to add a bot. For example:
    // if (m_pEngineServer) {
    //    m_pEngineServer->ServerCommand(("bot_add_named \"" + pendingBot.name + "\"").c_str()); // If engine supports this
    // } else {
    //    LogMessageToConsole("ERROR: No engine interface to add bot.");
    //    return false;
    // }
    // The team/class commands will be issued in FinalizeBotSetup AFTER the edict is known.

    return true;
}

void CRCBotPlugin::FinalizeBotSetup(edict_t* pBotEdict, const BotInfo& pendingBotInfo) {
    if (m_ManagedBots.count(pBotEdict)) {
        LogMessageToConsole(("Warning: Bot edict already managed: " + pendingBotInfo.name).c_str());
        return;
    }

    BotInfo newBotInfo = pendingBotInfo; // Copy skill, name, desired team/class
    newBotInfo.pEdict = pBotEdict;
    newBotInfo.isPendingSpawn = false;
    // The actual team/class might need to be confirmed from game state if jointeam/joinclass is not instant
    newBotInfo.assignedTeamId = pendingBotInfo.assignedTeamId; // Assume commands worked or will work
    newBotInfo.desiredClassId = pendingBotInfo.desiredClassId; // Assume commands worked
    newBotInfo.className = pendingBotInfo.className;


    // Issue commands to set team and class *now that we have an edict*
    // This is where these commands truly belong.
    std::string teamCmd = (newBotInfo.assignedTeamId == 2) ? "red" : "blue"; // Example mapping
    EngineIssueBotCommand(pBotEdict, "jointeam %s", teamCmd.c_str());
    EngineIssueBotCommand(pBotEdict, "joinclass %s", newBotInfo.className.c_str());


    // Find the ClassConfigInfo for this class
    const ClassConfigInfo* foundConfig = nullptr;
    for (const auto& cfg : m_ClassConfigs) {
        if (cfg.className == newBotInfo.className) { // Or match by classId
            foundConfig = &cfg;
            break;
        }
    }
    if (!foundConfig && !m_ClassConfigs.empty()) { // Fallback to first available config if specific not found
        foundConfig = &m_ClassConfigs[0];
        LogMessageToConsole(("Warning: Class config for " + newBotInfo.className + " not found. Using default.").c_str());
    } else if (!foundConfig) {
         LogMessageToConsole(("Error: No class configs loaded. Cannot create AI for " + newBotInfo.className).c_str());
         return; // Cannot proceed without class config
    }

    // Create CFFPlayer wrapper (conceptual - CFFPlayer needs to be defined)
    // CFFPlayer* pBotPlayerWrapper = new CFFPlayer(pBotEdict); // This would be owned by BotInfo or another manager

    // Instantiate CObjectivePlanner for the bot
    // The CFFPlayer wrapper would be passed to the planner and AI module.
    // For now, passing nullptr as pBotPlayer placeholder.
    newBotInfo.objectivePlanner = std::make_unique<CObjectivePlanner>(nullptr /*pBotPlayerWrapper*/, m_pKnowledgeBase.get());

    // Instantiate the AI module (e.g., CBotFortress or a class-specific one)
    newBotInfo.aiModule = AIFactory::CreateAIModule(
        newBotInfo.className,
        nullptr /*pBotPlayerWrapper*/,
        newBotInfo.objectivePlanner.get(), // Pass the planner to the AI
        m_pKnowledgeBase.get(),
        foundConfig
    );

    if (!newBotInfo.aiModule) {
        LogMessageToConsole(("Error: Could not create AI module for class " + newBotInfo.className).c_str());
        // delete pBotPlayerWrapper; // Clean up if AI module creation failed
        return; // Don't add to managed bots
    }
    // If CFFPlayer was newed: newBotInfo.pPlayerWrapper = pBotPlayerWrapper;

    m_ManagedBots[pBotEdict] = std::move(newBotInfo);
    LogMessageToConsole(("Bot finalized and managed: " + m_ManagedBots[pBotEdict].name + " (" + m_ManagedBots[pBotEdict].className + ")").c_str());
}

void CRCBotPlugin::OnBotLeaveGame(edict_t* pBotEdict) {
    auto it = m_ManagedBots.find(pBotEdict);
    if (it != m_ManagedBots.end()) {
        LogMessageToConsole(("Bot leaving: " + it->second.name).c_str());
        m_ManagedBots.erase(it);
    }
    // Also remove from pending if it was there and never finalized
    m_PendingBots.erase(std::remove_if(m_PendingBots.begin(), m_PendingBots.end(),
        [&](const BotInfo& bi) { return bi.name == playername_from_edict(pBotEdict); /* conceptual playername_from_edict */ }),
        m_PendingBots.end());
}


// --- Plugin Specific Logic ---
void CRCBotPlugin::PluginCycle() {
    // Conceptual: if (!gpGlobals) return;
    // Conceptual: float deltaTime = gpGlobals->frametime;

    for (auto& pair : m_ManagedBots) {
        BotInfo& botInfo = pair.second;
        edict_t* pBotEdict = pair.first; // Or botInfo.pEdict

        if (botInfo.isPendingSpawn) continue; // Don't update bots not fully in game yet

        if (botInfo.aiModule && botInfo.objectivePlanner) {
            // Ensure CFFPlayer conceptual object is valid if used by AI
            // CFFPlayer* botPlayer = GetCFFPlayerFromEdict(pBotEdict); // Conceptual
            // if (!botPlayer || !botPlayer->IsAlive()) continue;

            UserCmd currentCmd; // Create a new command for this bot for this frame

            // 1. Planner evaluates and selects/updates high-level task & subtasks
            //    The planner might internally check if m_CurrentHighLevelTask is still valid
            //    or if all subtasks are done, then call Generate/Prioritize/Select/Decompose.
            botInfo.objectivePlanner->EvaluateAndSelectTask();

            // 2. AI Module (CBotFortress) executes its Update logic,
            //    which internally gets the current subtask from its planner and acts.
            //    The CBotFortress::Update method will populate currentCmd.
            //    Note: In RCBot2, CBot::Upkeep() is the main AI loop, which calls
            //    m_pSchedules->execute(this), which in turn calls tasks' execute.
            //    So, botInfo.aiModule->Update(pCmd) is equivalent to CBot::Upkeep() or CBot::RunAI().
            botInfo.aiModule->Update(&currentCmd);

            // 3. (Conceptual) Apply the populated UserCmd to the bot entity in the game.
            //    This is highly engine-specific.
            //    Example: engine->SetUserCmd(pBotEdict, currentCmd);
            //    Example: or if bots are fake clients: gameServer->SetFakeClientUserCmd(pBotEdict, currentCmd);
            //    LogMessageToConsole(("Bot " + botInfo.name + " Cmd: Fwd:" + std::to_string(currentCmd.forwardmove) + " Btns:" + std::to_string(currentCmd.buttons)).c_str());

        } else {
            // Log warning: Bot is managed but missing AI or Planner
            // LogMessageToConsole(("Warning: Bot " + botInfo.name + " is missing AI components.").c_str());
        }
    }
}

// --- Lua Bridge Management ---
bool CRCBotPlugin::FindLuaState() {
    // Placeholder - in a real scenario, this would try to get the game's lua_State*
    if (!m_pLuaState) { // Only create if not found (or for testing)
        LogMessageToConsole("INFO: Game lua_State not found/set. Creating new Lua state for plugin testing.");
        m_pLuaState = luaL_newstate();
        if (m_pLuaState) {
            luaL_openlibs(m_pLuaState);
        } else {
            LogMessageToConsole("ERROR: luaL_newstate() failed!");
            return false;
        }
    }
    return (m_pLuaState != nullptr);
}

bool CRCBotPlugin::InitializeLuaBridge(const char* mapName) {
    // (Implementation from previous step, ensure m_ControlPoints etc. are populated)
    if (!FindLuaState()) {
        LogMessageToConsole("ERROR: Could not find or initialize Lua state for bridge.");
        return false;
    }
    RegisterLuaFunctions();

    // Execute a general bot initialization script
    if (!ExecuteLuaScript("scripts/rcbot_main_config.lua")) {
        LogMessageToConsole("Warning: Failed to execute scripts/rcbot_main_config.lua. Class configs might be missing.");
    }

    // Load map-specific data script AFTER general configs
    if (mapName && mapName[0] != '\0') {
        char mapScriptPath[256];
        snprintf(mapScriptPath, sizeof(mapScriptPath), "scripts/maps/data/%s_data.lua", mapName);
        if (!ExecuteLuaScript(mapScriptPath)) {
            LogMessageToConsole(("Warning: Failed to execute map-specific script: " + std::string(mapScriptPath)).c_str());
        }
    }

    // Populate Class Configs (assuming defined in rcbot_main_config.lua or included files)
    m_ClassConfigs.clear();
    // This part needs Lua to provide a list of class config tables to load.
    // For now, using hardcoded example from before, but this should be dynamic.
    lua_getglobal(m_pLuaState, "g_ClassConfigTableNames"); // Expects a global Lua table of strings
    if (lua_istable(m_pLuaState, -1)) {
        lua_pushnil(m_pLuaState); // First key
        while (lua_next(m_pLuaState, -2) != 0) { // key at -2, value at -1
            if (lua_isstring(m_pLuaState, -1)) {
                const char* tableName = lua_tostring(m_pLuaState, -1);
                ClassConfigInfo cfg;
                if (PopulateClassConfigInfo(m_pLuaState, cfg, tableName)) {
                    m_ClassConfigs.push_back(cfg);
                    LogMessageToConsole(("Loaded Lua Class Config: " + std::string(tableName)).c_str());
                } else {
                    LogMessageToConsole(("WARNING: Failed to load Lua Class Config: " + std::string(tableName)).c_str());
                }
            }
            lua_pop(m_pLuaState, 1); // Remove value, keep key for next iteration
        }
    } else {
         LogMessageToConsole("Warning: g_ClassConfigTableNames Lua table not found. Using hardcoded test configs.");
        // Fallback to test configs if global table not found
        const char* testClassTables[] = {"g_ClassConfig_Scout_Test", "g_ClassConfig_Soldier_Test"};
        for(const char* tableName : testClassTables){
            ClassConfigInfo cfg;
            if(PopulateClassConfigInfo(m_pLuaState, cfg, tableName)){ m_ClassConfigs.push_back(cfg); }
        }
    }
    lua_pop(m_pLuaState, 1); // Pop g_ClassConfigTableNames or nil

    if (m_pKnowledgeBase) m_pKnowledgeBase->classConfigs = &m_ClassConfigs;


    // Populate Control Points (assuming defined in map_data.lua)
    m_ControlPoints.clear();
    // Similar dynamic loading for CPs based on map data in Lua
    // For now, one test CP:
    ControlPointInfo cpTest;
    if (PopulateControlPointInfo(m_pLuaState, cpTest, "g_ControlPoint_TestCP1")) {
         m_ControlPoints.push_back(cpTest);
    }
    if (m_pKnowledgeBase) m_pKnowledgeBase->controlPoints = &m_ControlPoints;


    m_bLuaBridgeInitialized = true;
    LogMessageToConsole("Lua bridge initialized.");
    return true;
}

void CRCBotPlugin::ShutdownLuaBridge() {
    m_bLuaBridgeInitialized = false;
    // if (m_pLuaState && we_own_this_lua_state) { lua_close(m_pLuaState); } // Be careful
    m_pLuaState = nullptr;
    LogMessageToConsole("Lua bridge shutdown.");
}

bool CRCBotPlugin::ExecuteLuaScript(const char* scriptPath) {
    if (!m_pLuaState) {
        LogMessageToConsole(("Error: Lua state not available to execute " + std::string(scriptPath)).c_str());
        return false;
    }
    if (luaL_dofile(m_pLuaState, scriptPath) != LUA_OK) {
        const char* luaError = lua_tostring(m_pLuaState, -1);
        LogMessageToConsole(("Error executing Lua script '" + std::string(scriptPath) + "': " + (luaError ? luaError : "unknown")).c_str());
        lua_pop(m_pLuaState, 1); // Pop error message
        return false;
    }
    LogMessageToConsole(("Executed Lua script: " + std::string(scriptPath)).c_str());
    return true;
}

// --- C++ Functions Exposed to Lua ---
// (Static wrappers and member function implementations from previous step)
// ...
void CRCBotPlugin::LogMessageToConsole(const char* message) {
    // Use engine logging if m_pEngineServer is available
    if (m_pEngineServer && false) { // Disabled for now to avoid compile error if IVEngineServer is not fully defined
        // m_pEngineServer->ConMsg("[RCBot]: %s\n", message);
    } else {
        std::cout << "[RCBot]: " << message << std::endl;
    }
}
float CRCBotPlugin::GetCurrentGameTime() const {
    // if (m_pGlobals) return m_pGlobals->curtime;
    return 0.0f;
}
const char* CRCBotPlugin::GetCurrentMapName() const { return m_CurrentMapName; }


void CRCBotPlugin::RegisterLuaFunctions() { /* ... (from previous step) ... */ }


// --- Engine Helper Implementations ---
int CRCBotPlugin::GetTeamPlayerCount(int teamId) const {
    int count = 0;
    // Conceptual: Iterate through actual game players
    // if (m_pServerGameClients && m_pGlobals) {
    //     for (int i = 1; i <= m_pGlobals->maxClients; ++i) {
    //         edict_t* pPlayerEdict = m_pEngineServer->PEntityOfEntIndex(i);
    //         if (pPlayerEdict && !pPlayerEdict->IsFree()) {
    //             // CBasePlayer* pPlayer = (CBasePlayer*)pPlayerEdict->GetUnknown(); // Highly game specific
    //             // if (pPlayer && pPlayer->GetTeamNumber() == teamId && !pPlayer->IsBot() /* or include bots */) {
    //             //     count++;
    //             // }
    //         }
    //     }
    // }
    // Fallback for testing if actual player iteration isn't available:
    for (const auto& pair : m_ManagedBots) {
        if (pair.second.assignedTeamId == teamId && !pair.second.isPendingSpawn) {
            count++;
        }
    }
    return count;
}

void CRCBotPlugin::EngineIssueBotCommand(edict_t* pBotEdict, const char* commandFormat, ...) {
    if (!pBotEdict && !true /*!IsSourceDedicatedServer() - this check is if commands are global vs client specific*/) {
        // This is a simplification. Real bot command execution is engine-specific.
        // It might involve finding the client index for pBotEdict and using engine->ClientCommand().
        // Or, if bots are "fake clients", there might be a specific function to issue commands to them.
        LogMessageToConsole("Warning: EngineIssueBotCommand called with NULL edict in a context that might require it.");
        // return;
    }

    char commandBuffer[256];
    va_list args;
    va_start(args, commandFormat);
    vsnprintf(commandBuffer, sizeof(commandBuffer), commandFormat, args);
    va_end(args);

    // Conceptual: This is where the engine API call would go.
    // Example: if (m_pEngineServer && pBotEdict) m_pEngineServer->ClientCommand(pBotEdict, "%s\n", commandBuffer);
    // Example: else if (m_pEngineServer) m_pEngineServer->ServerCommand( "%s\n", commandBuffer); // If command is server-wide for bot
    LogMessageToConsole(("ENGINE BOT CMD (Conceptual) for Edict [%p]: %s", (void*)pBotEdict, commandBuffer));
}

// Placeholder for CFFBaseAI::CreateAIModule - this would typically be a factory in CFFBaseAI.cpp
// std::unique_ptr<CFFBaseAI> CFFBaseAI::CreateAIModule(const std::string& className, CFFPlayer* pPlayer, BotKnowledgeBase* kb, const ClassConfigInfo* classCfg) {
//     if (className == "scout") return std::make_unique<CScoutAI>(pPlayer, kb, classCfg);
//     // ... etc.
//     return nullptr;
// }
