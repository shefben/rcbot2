#include "CRCBotPlugin.h"
#include "FFBaseAI.h"
#include "ObjectivePlanner.h"
#include "FFStateStructs.h"   // For Vector, ClassConfigInfo, etc. (used by BotKnowledgeBase)
#include "BotTasks.h"         // For SubTask, CUserCmd (if custom struct is there)

// Conceptual SDK includes (replace with actual ones)
#include <iostream> // For placeholder logging
#include <algorithm> // For std::remove_if, std::find_if
#include <stdarg.h>  // For va_list if EngineIssueBotCommand were used

// --- Conceptual Placeholder Types (if not properly included from SDK) ---
// These allow CRCBotPlugin and its composed classes to "compile" conceptually.
#ifndef CUSERCMD_CONCEPTUAL_DEF_CRCBOTPLUGIN
#define CUSERCMD_CONCEPTUAL_DEF_CRCBOTPLUGIN
struct CUserCmd {
    int buttons = 0; float forwardmove = 0.0f; float sidemove = 0.0f; Vector viewangles; int weaponselect = 0;
    // Minimal UserCmd. Add impulse, mousedx, etc. as needed by AI.
};
#endif

#ifndef CBASEENTITY_CONCEPTUAL_DEF_CRCBOTPLUGIN
#define CBASEENTITY_CONCEPTUAL_DEF_CRCBOTPLUGIN
class CBaseEntity {
public:
    virtual ~CBaseEntity() {}
    Vector GetPosition() const { return Vector(0,0,0); }
    Vector GetWorldSpaceCenter() const { return Vector(0,0,0); }
    bool IsAlive() const { return true; }
    int GetTeamNumber() const { return 0; }
    // Add other necessary methods like GetMaxHealth, GetHealth, GetFlags, etc.
};
#endif

#ifndef CFFPLAYER_CONCEPTUAL_DEF_CRCBOTPLUGIN
#define CFFPLAYER_CONCEPTUAL_DEF_CRCBOTPLUGIN
class CFFPlayer {
public:
    CFFPlayer(edict_t* ed) : m_pEdict(ed) {}
    edict_t* GetEdict() const { return m_pEdict; }
    bool IsAlive() const { return true; }
    Vector GetPosition() const { return Vector(0,0,0); }
    Vector GetEyePosition() const { return Vector(0,0,64); }
    Vector GetViewAngles() const { return Vector(0,0,0); }
    void SetViewAngles(const Vector& ang) { /* Sets bot's view */ }
    int GetTeamNumber() const { return 1; }
    int GetFlags() const { return 1; } // FL_ONGROUND
private:
    edict_t* m_pEdict;
};
#endif

// If edict_t is not defined by eiface.h (it usually is)
#ifndef EDICT_T_CONCEPTUAL_DEF
#define EDICT_T_CONCEPTUAL_DEF
// struct edict_t { int id; /* dummy */ }; // Already forward declared in .h
#endif

// Global plugin instance (if using EXPOSE_SINGLE_INTERFACE_GLOBALVAR)
// CRCBotPlugin g_CRCBotPlugin; // Definition for the extern in .h

// --- CRCBotPlugin Implementation ---

CRCBotPlugin::CRCBotPlugin()
    : m_NextBotIdCounter(1)
    // m_pEngineServer(nullptr),
    // m_pGlobalKnowledgeBase(nullptr) // unique_ptr initializes to nullptr
{
    // g_pRCBotPlugin = this; // If using a global pointer to the plugin instance
    std::cout << "CRCBotPlugin: Constructor." << std::endl;
}

CRCBotPlugin::~CRCBotPlugin() {
    std::cout << "CRCBotPlugin: Destructor." << std::endl;
    // m_ManagedBots unique_ptrs will auto-cleanup AI and Planner instances
}

//---------------------------------------------------------------------------------
// Purpose: called when the plugin is loaded, load the interface we need from the engine
//---------------------------------------------------------------------------------
bool CRCBotPlugin::Load(CreateInterfaceFn interfaceFactory, CreateInterfaceFn gameServerFactory) {
    std::cout << "CRCBotPlugin: Load called." << std::endl;
    m_NextBotIdCounter = 1;

    // Conceptual: Get engine interfaces
    // m_pEngineServer = (IVEngineServer*)interfaceFactory(INTERFACEVERSION_VENGINESERVER, nullptr);
    // m_pPlayerInfoManager = (IPlayerInfoManager*)interfaceFactory(INTERFACEVERSION_PLAYERINFOMANAGER, nullptr);
    // m_pServerGameClients = (IServerGameClients*)interfaceFactory(INTERFACEVERSION_SERVERGAMECLIENTS, nullptr);
    // m_pBotManager = (IBotManager*)interfaceFactory(INTERFACEVERSION_BOTMANAGER, nullptr); // If engine has one
    // m_pCvar = (ICvar*)interfaceFactory(CVAR_INTERFACE_VERSION, nullptr);

    // if (!m_pEngineServer || !m_pPlayerInfoManager || !m_pServerGameClients /* || !m_pBotManager */ || !m_pCvar) {
    //     std::cerr << "CRCBotPlugin: Failed to get one or more required engine interfaces." << std::endl;
    //     return false;
    // }

    // Initialize global knowledge base (if not already)
    // m_pGlobalKnowledgeBase = std::make_unique<BotKnowledgeBase>();
    // if (m_pGlobalKnowledgeBase) {
    //     m_pGlobalKnowledgeBase->classConfigs = &m_GlobalClassConfigs;
    //     m_pGlobalKnowledgeBase->controlPoints = &m_ControlPoints;
    //     m_pGlobalKnowledgeBase->payloadPaths = &m_PayloadPaths;
    // }

    // Conceptual: Register console commands for adding/removing bots
    // m_pCvar->RegisterConCommand(new ConCommand("rcbot_add", &StaticAddBotCommand, "Adds a bot.", FCVAR_CHEAT));

    std::cout << "CRCBotPlugin: Loaded successfully." << std::endl;
    return true;
}

//---------------------------------------------------------------------------------
// Purpose: called when the plugin is unloaded
//---------------------------------------------------------------------------------
void CRCBotPlugin::Unload() {
    std::cout << "CRCBotPlugin: Unload called." << std::endl;
    LevelShutdown(); // Clear bots and map data
    m_ManagedBots.clear();
    // m_pGlobalKnowledgeBase.reset(); // Release unique_ptr
    // Conceptual: Unregister ConCommands
}

void CRCBotPlugin::Pause() { std::cout << "CRCBotPlugin: Pause." << std::endl; }
void CRCBotPlugin::UnPause() { std::cout << "CRCBotPlugin: UnPause." << std::endl; }
const char *CRCBotPlugin::GetPluginDescription() { return "RCBot FortressForever Plugin"; }

void CRCBotPlugin::LevelInit(char const *pMapName) {
    std::cout << "CRCBotPlugin: LevelInit for map: " << (pMapName ? pMapName : "N/A") << std::endl;
    m_ManagedBots.clear();
    m_NextBotIdCounter = 1;

    // Conceptual: Load navmesh for pMapName into m_pGlobalKnowledgeBase->navMesh
    // Conceptual: Load ClassConfigInfo from Lua into m_GlobalClassConfigs
    // Conceptual: Load ControlPointInfo, PayloadPathInfo for pMapName from Lua into respective vectors
    // Example: if(InitializeLuaBridge(pMapName)) { /* load data using FFLuaBridge */ }
}

void CRCBotPlugin::LevelShutdown() {
    std::cout << "CRCBotPlugin: LevelShutdown." << std::endl;
    m_ManagedBots.clear(); // Destroys BotInfo objects, which destroys AI and Planner unique_ptrs
    // m_ControlPoints.clear();
    // m_PayloadPaths.clear();
    // if (m_pGlobalKnowledgeBase && m_pGlobalKnowledgeBase->navMesh) {
    //    m_pGlobalKnowledgeBase->navMesh->Clear();
    // }
    // ShutdownLuaBridge();
}

void CRCBotPlugin::ServerActivate(edict_t *pEdictList, int edictCount, int clientMax) {
    std::cout << "CRCBotPlugin: ServerActivate." << std::endl;
}

void CRCBotPlugin::GameFrame(bool simulating) {
    if (simulating) {
        UpdateAllBots();
        // s_fConceptualGameTime += 0.015f; // Advance conceptual time if used by tasks
    }
}

void CRCBotPlugin::ClientActive(edict_t *pEntity) {
    // std::cout << "CRCBotPlugin: ClientActive for edict " << pEntity << std::endl;
    // This is often called *after* ClientPutInServer. Useful for some post-spawn logic.
}

void CRCBotPlugin::ClientDisconnect(edict_t *pEntity) {
    // std::cout << "CRCBotPlugin: ClientDisconnect for edict " << pEntity << std::endl;
    RemoveBot(pEntity);
}

void CRCBotPlugin::ClientPutInServer(edict_t *pEntity, char const *playername) {
    std::cout << "CRCBotPlugin: ClientPutInServer - Name: " << playername << ", Edict: " << pEntity << std::endl;
    // Check if this playername matches any of our pending bots
    for (BotInfo& botInfo : m_ManagedBots) { // Iterate by reference
        if (botInfo.isPendingSpawn && botInfo.name == playername) {
            FinalizeBotAddition(pEntity, botInfo.name, botInfo.teamId, botInfo.classNameRequest, botInfo.skillLevel /* conceptual skill */);
            // Now that we have an edict, update the BotInfo entry
            botInfo.pEdict = pEntity;
            botInfo.isPendingSpawn = false; // No longer pending, edict assigned
            // The rest of AI/Planner setup is in FinalizeBotAddition logic (which is now mostly inside this func)
            return; // Found and processed our pending bot
        }
    }
}

void CRCBotPlugin::SetCommandClient(int index) {}
void CRCBotPlugin::ClientSettingsChanged(edict_t *pEdict) {}

PLUGIN_RESULT CRCBotPlugin::ClientConnect(bool *bAllowConnect, edict_t *pEntity, const char *pszName, const char *pszAddress, char *reject, int maxrejectlen) {
    return PLUGIN_CONTINUE;
}

PLUGIN_RESULT CRCBotPlugin::ClientCommand(edict_t *pEntity, const void*& args /* const CCommand &args */) {
    // Conceptual: Handle bot commands if they are sent via ClientCommand
    // const CCommand* pCmd = reinterpret_cast<const CCommand*>(args);
    // if (pCmd && pEntity && GetBotInfoByEdict(pEntity)) { // If it's one of our bots
    //     if (strcmp(pCmd->Arg(0), "rcbot_custom_cmd") == 0) { /* handle it */ return PLUGIN_STOP; }
    // }
    return PLUGIN_CONTINUE;
}

PLUGIN_RESULT CRCBotPlugin::NetworkIDValidated(const char *pszUserName, const char *pszNetworkID) {
    return PLUGIN_CONTINUE;
}

void CRCBotPlugin::OnQueryCvarValueFinished(int iCookie, edict_t *pPlayerEntity, int eStatus, const char *pCvarName, const char *pCvarValue) {}
void CRCBotPlugin::OnEdictAllocated(edict_t *edict) {}
void CRCBotPlugin::OnEdictFreed(const edict_t *edict) {
    // If this edict was a bot we managed, ensure it's cleaned up.
    // ClientDisconnect should usually handle this, but this is a fallback.
    RemoveBot(const_cast<edict_t*>(edict)); // RCBot2 used const_cast here
}

// --- Bot Management Implementation ---

int CRCBotPlugin::RequestBot(const std::string& name, int teamId, const std::string& className, int skill) {
    std::string botName = name;
    if (botName.empty()) {
        botName = "RCBot_" + std::to_string(m_NextBotIdCounter);
    }

    int currentBotId = m_NextBotIdCounter++;

    // Add to m_ManagedBots with pEdict=nullptr and isPendingSpawn=true.
    // ClientPutInServer will call FinalizeBotAddition when the engine confirms the spawn.
    m_ManagedBots.emplace_back(currentBotId, botName, teamId, className); // Uses new BotInfo constructor
    // BotInfo& newBotRequest = m_ManagedBots.back(); // This reference might be invalidated if vector reallocates
                                                 // Better to find it again in ClientPutInServer or pass necessary info.

    // Conceptual: Tell the engine to add a bot. This is the KEY engine interaction.
    // Example: if (m_pEngineServer && m_pBotManager) { m_pBotManager->CreateFakeClient(botName.c_str()); }
    // Or: if (m_pEngineServer) { m_pEngineServer->ServerCommand(("bot_add_named \"" + botName + "\" team " + std::to_string(teamId) + " class " + className).c_str()); }
    std::cout << "CRCBotPlugin: Requested bot " << botName << " (ID: " << currentBotId
              << ", Team: " << teamId << ", Class: " << className
              << "). Waiting for engine to call ClientPutInServer." << std::endl;

    // For testing without a real engine that calls ClientPutInServer:
    // edict_t* dummyEdict = new edict_t{currentBotId}; // DANGER: LEAKS if not deleted in RemoveBot
    // ClientPutInServer(dummyEdict, botName.c_str()); // Simulate callback to finalize.

    return currentBotId;
}

// This function is now effectively merged into ClientPutInServer's logic for matching pending bots.
// Or, it can be a helper if ClientPutInServer just finds the BotInfo and passes it here.
void CRCBotPlugin::FinalizeBotAddition(edict_t* pBotEdict, const std::string& botName, int teamId, const std::string& className, int skill) {
    BotInfo* pBotInfo = nullptr;
    for(BotInfo& bi : m_ManagedBots) {
        if(bi.name == botName && bi.isPendingSpawn) {
            pBotInfo = &bi;
            break;
        }
    }

    if (!pBotInfo) {
        std::cerr << "CRCBotPlugin: FinalizeBotAddition - Could not find pending bot info for: " << botName << std::endl;
        // Potentially kick this pBotEdict if it's not meant to be here.
        return;
    }

    pBotInfo->pEdict = pBotEdict;
    pBotInfo->isPendingSpawn = false;
    pBotInfo->teamId = teamId; // Confirm or set team
    pBotInfo->classNameRequest = className; // Confirm or set class
    // pBotInfo->internalClassId = ResolveClassNameToGameId(className); // Conceptual
    pBotInfo->skillLevel = skill; // Conceptual

    std::cout << "CRCBotPlugin: Finalizing bot " << pBotInfo->name << " with edict " << pEdict << std::endl;

    const ClassConfigInfo* selectedClassConfig = nullptr;
    // if (m_pGlobalKnowledgeBase && m_pGlobalKnowledgeBase->classConfigs) {
    //     for (const auto& cfg : *m_pGlobalKnowledgeBase->classConfigs) {
    //         if (cfg.className == pBotInfo->classNameRequest /* || cfg.classId == pBotInfo->internalClassId */) {
    //             selectedClassConfig = &cfg;
    //             break;
    //         }
    //     }
    // }
    // if (!selectedClassConfig && m_pGlobalKnowledgeBase && m_pGlobalKnowledgeBase->classConfigs && !m_pGlobalKnowledgeBase->classConfigs->empty()) {
    //     selectedClassConfig = &m_pGlobalKnowledgeBase->classConfigs->front();
    // }
     if (!selectedClassConfig) {
         std::cout << "Warning: ClassConfig for '" << pBotInfo->classNameRequest << "' not found for bot " << pBotInfo->name << ". AI may be limited." << std::endl;
    }

    // CFFPlayer* playerWrapper = new CFFPlayer(pBotEdict); // Conceptual
    pBotInfo->objectivePlanner = std::make_unique<CObjectivePlanner>(nullptr /*playerWrapper*/, m_pGlobalKnowledgeBase.get());
    pBotInfo->aiModule = AIFactory::CreateAIModule(
        pBotInfo->classNameRequest,
        nullptr /*playerWrapper*/,
        pBotInfo->objectivePlanner.get(),
        m_pGlobalKnowledgeBase.get(),
        selectedClassConfig
    );

    if (!pBotInfo->aiModule) {
        std::cout << "CRCBotPlugin: ERROR - Failed to create AI module for " << pBotInfo->name << std::endl;
        pBotInfo->isActive = false;
        return;
    }

    pBotInfo->isActive = true;
    std::cout << "CRCBotPlugin: Bot " << pBotInfo->name << " (ID: " << pBotInfo->botId << ") is now active." << std::endl;

    // Conceptual: Make the bot join team and class via engine commands if not handled by bot_add
    // std::string teamCmdStr = (teamId == 2) ? "red" : "blue";
    // EngineClientCommand(pBotEdict, "jointeam %s", teamCmdStr.c_str());
    // EngineClientCommand(pBotEdict, "joinclass %s", className.c_str());
}

void CRCBotPlugin::RemoveBot(edict_t* pBotEdict) {
    auto it = std::remove_if(m_ManagedBots.begin(), m_ManagedBots.end(),
                             [pBotEdict](const BotInfo& bi) { return bi.pEdict == pBotEdict; });

    if (it != m_ManagedBots.end()) {
        std::cout << "CRCBotPlugin: Removing bot " << it->name << " (ID: " << it->botId << ") with edict " << pBotEdict << std::endl;
        // if (it->pEdict) { /* delete it->pEdict; */ } // Only if we new'd dummy edicts for testing
        m_ManagedBots.erase(it, m_ManagedBots.end());
    }
}

void CRCBotPlugin::UpdateAllBots() { // Removed CUserCmd* pUserCmdArray parameter
    for (BotInfo& bot : m_ManagedBots) { // Iterate by reference
        if (bot.isActive && bot.aiModule && bot.objectivePlanner && bot.pEdict /* && bot.playerWrapper->IsAlive() */) {
            CUserCmd botCmd;

            if (bot.objectivePlanner) {
                bot.objectivePlanner->EvaluateAndSelectTask();
            }

            if (bot.aiModule) {
                bot.aiModule->Update(&botCmd);
            }

            // Conceptual: Apply botCmd to the bot's entity in the game
            // if (m_pEngineServer && bot.pEdict) {
            //    m_pEngineServer->SetFakeClientUserCmd(bot.pEdict, &botCmd); // Example engine call
            // }
            // if (botCmd.buttons != 0) {
            //    std::cout << "Updating bot " << bot.name << ", Cmd Btns: " << botCmd.buttons << std::endl;
            // }
        }
    }
}

BotInfo* CRCBotPlugin::GetBotInfoByEdict(edict_t* pEdict) {
    for (BotInfo& botInfo : m_ManagedBots) {
        if (botInfo.pEdict == pEdict) {
            return &botInfo;
        }
    }
    return nullptr;
}
