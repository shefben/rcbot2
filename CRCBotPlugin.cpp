#include "CRCBotPlugin.h"
#include "FFBaseAI.h"         // For CFFBaseAI, BotKnowledgeBase (which includes ff_state_structs.h)
#include "ObjectivePlanner.h" // For CObjectivePlanner
// #include "engine_interfaces.h" // Conceptual: where IVEngineServer etc. would be included
// #include "game_rules.h"        // Conceptual: for game mode, team scores etc.

// Conceptual: These would typically come from actual SDK headers
#ifndef M_PI // Ensure M_PI is defined for std::atan2, std::cos if not already
#define M_PI 3.14159265358979323846
#endif
#include <cmath> // For atan2, sqrt, etc.

// For placeholder logging
#include <iostream>
#include <algorithm> // For std::remove_if, std::sort
#include <stdarg.h>  // For va_list in EngineIssueBotCommand (if that were to be re-added)


// --- Conceptual Placeholder Types (would come from engine/game SDK) ---
// These are minimal to allow CRCBotPlugin and its composed classes to "compile" conceptually.
// In a real integration, these would be replaced by actual engine/SDK types and headers.

// struct edict_t moved to CRCBotPlugin.h for BotInfo, but it's an engine type.
// If CRCBotPlugin.h doesn't define it, it needs to be defined or included before.
// For now, assume it's adequately handled by being in CRCBotPlugin.h's fwd declaration context.

// CUserCmd needs to be defined before FFBaseAI.h uses it, or forward declared and then defined.
// Let's put a minimal definition here if FFBaseAI.cpp also needs it.
// It was also in FFBaseAI.cpp - should be in one common conceptual place or from SDK.
#ifndef USERCMD_CONCEPTUAL_DEF
#define USERCMD_CONCEPTUAL_DEF
struct CUserCmd {
    int buttons = 0;
    float forwardmove = 0.0f;
    float sidemove = 0.0f;
    Vector viewangles;
    // int weaponselect; // Example field
};
#endif

// CBaseEntity placeholder
#ifndef CBASEENTITY_CONCEPTUAL_DEF
#define CBASEENTITY_CONCEPTUAL_DEF
class CBaseEntity {
public:
    virtual ~CBaseEntity() {}
    virtual Vector GetPosition() const { return Vector(0,0,0); }
    virtual Vector GetWorldSpaceCenter() const { return Vector(0,0,32); }
    virtual bool IsAlive() const { return true; }
    virtual int GetTeamNumber() const { return 0; }
    // virtual std::string GetClassName() const { return "unknown_entity"; }
};
#endif

// CFFPlayer placeholder (wrapper around edict_t or CBaseEntity*)
#ifndef CFFPLAYER_CONCEPTUAL_DEF
#define CFFPLAYER_CONCEPTUAL_DEF
class CFFPlayer {
public:
    CFFPlayer(edict_t* ed) : m_pEdict(ed) {}
    edict_t* GetEdict() const { return m_pEdict; }
    bool IsAlive() const {
        // Conceptual: query from edict or game state
        return true;
    }
    Vector GetPosition() const {
        // Conceptual: query from edict or game state
        return Vector(0,0,0);
    }
    Vector GetEyePosition() const { return Vector(0,0,64); }
    // QAngle GetViewAngles() const;
    // void SetViewAngles(const QAngle& ang);
    int GetTeamNumber() const { return 0;} // Conceptual
    // ... other methods to get health, ammo, active weapon, etc.
private:
    edict_t* m_pEdict;
};
#endif

// --- Global Plugin Pointer ---
CRCBotPlugin* g_pRCBotPlugin = nullptr; // Definition

// --- CRCBotPlugin Implementation ---

CRCBotPlugin::CRCBotPlugin()
    : m_NextBotIdCounter(1),
      m_pGlobalKnowledgeBase(nullptr)
      // m_pEngineServer(nullptr) // Initialize interface pointers if they are members
{
    g_pRCBotPlugin = this;
    std::cout << "CRCBotPlugin: Constructor" << std::endl;
}

CRCBotPlugin::~CRCBotPlugin() {
    std::cout << "CRCBotPlugin: Destructor" << std::endl;
    m_ManagedBots.clear(); // Ensures unique_ptrs in BotInfo are destructed
    g_pRCBotPlugin = nullptr;
}

bool CRCBotPlugin::Load(CreateInterfaceFn interfaceFactory, CreateInterfaceFn gameServerFactory) {
    std::cout << "CRCBotPlugin: Load" << std::endl;
    m_NextBotIdCounter = 1;

    // Conceptual: Get engine interfaces here
    // m_pEngineServer = (IVEngineServer*)interfaceFactory(VENGINESERVER_INTERFACE_VERSION, nullptr);
    // if (!m_pEngineServer) { /* handle error */ }

    m_pGlobalKnowledgeBase = std::make_unique<BotKnowledgeBase>();
    if (m_pGlobalKnowledgeBase) {
        // Conceptual: Load navmesh (map-agnostic parts or common data)
        // m_pGlobalKnowledgeBase->navMesh = new NavMeshGraph(); // If NavMeshGraph is heap-alloc
        // LoadClassConfigsFromLua(); // Populates m_GlobalClassConfigs
        m_pGlobalKnowledgeBase->classConfigs = &m_GlobalClassConfigs; // Link to plugin's storage
        m_pGlobalKnowledgeBase->controlPoints = &m_ControlPoints;   // Link (will be empty until LevelInit)
        m_pGlobalKnowledgeBase->payloadPaths = &m_PayloadPaths;   // Link (will be empty until LevelInit)

    } else {
        std::cerr << "CRCBotPlugin: Failed to allocate GlobalKnowledgeBase!" << std::endl;
        return false;
    }
    return true;
}

void CRCBotPlugin::Unload() {
    std::cout << "CRCBotPlugin: Unload" << std::endl;
    LevelShutdown(); // Clear bots and map-specific data
    // if (m_pGlobalKnowledgeBase && m_pGlobalKnowledgeBase->navMesh) {
    //     delete m_pGlobalKnowledgeBase->navMesh;
    //     m_pGlobalKnowledgeBase->navMesh = nullptr;
    // }
    m_pGlobalKnowledgeBase.reset(); // Release knowledge base
}

void CRCBotPlugin::GameFrame(bool simulating) {
    if (simulating) {
        UpdateAllBots(nullptr);
    }
}

void CRCBotPlugin::LevelInit(char const *pMapName) {
    std::cout << "CRCBotPlugin: LevelInit for map " << (pMapName ? pMapName : "null") << std::endl;
    m_ManagedBots.clear(); // Clear bots from previous level
    m_NextBotIdCounter = 1;

    if (m_pGlobalKnowledgeBase) {
        // Conceptual: Load map-specific navmesh
        // if (!m_pGlobalKnowledgeBase->navMesh) m_pGlobalKnowledgeBase->navMesh = new NavMeshGraph();
        // m_pGlobalKnowledgeBase->navMesh->LoadForMap(pMapName);

        // Conceptual: Load map objectives (CPs, Payload paths) from Lua via bridge
        // This would populate m_ControlPoints, m_PayloadPaths which are linked to KB
        // InitializeLuaBridgeAndLoadMapData(pMapName);
    }
}

void CRCBotPlugin::LevelShutdown() {
    std::cout << "CRCBotPlugin: LevelShutdown" << std::endl;
    m_ManagedBots.clear(); // Ensures AI/Planner unique_ptrs are destructed
    m_ControlPoints.clear(); // Clear map-specific data
    m_PayloadPaths.clear();  // Clear map-specific data

    // if (m_pGlobalKnowledgeBase && m_pGlobalKnowledgeBase->navMesh) {
    //    m_pGlobalKnowledgeBase->navMesh->Clear();
    // }
}

void CRCBotPlugin::ClientPutInServer(edict_t *pEntity, char const *playername) {
    std::cout << "CRCBotPlugin: ClientPutInServer - Name: " << playername << ", Edict: " << pEntity << std::endl;
    for (BotInfo& botInfo : m_ManagedBots) { // Iterate by reference
        if (botInfo.isPendingPlayerSlot && botInfo.name == playername) {
            FinalizeBotAddition(botInfo, pEntity);
            return;
        }
    }
}

void CRCBotPlugin::ClientDisconnect(edict_t *pEntity) {
    auto it = std::remove_if(m_ManagedBots.begin(), m_ManagedBots.end(),
                             [pEntity](const BotInfo& bi){ return !bi.isPendingPlayerSlot && bi.pEdict == pEntity; });

    if (it != m_ManagedBots.end()) {
        std::cout << "CRCBotPlugin: Bot " << it->name << " (ID: " << it->botId << ") disconnected/removed." << std::endl;
        m_ManagedBots.erase(it, m_ManagedBots.end());
    }
}

int CRCBotPlugin::RequestBot(const std::string& name, int team, const std::string& className) {
    std::string botName = name;
    if (botName.empty()) {
        botName = "RCBot_" + std::to_string(m_NextBotIdCounter);
    }

    int currentBotId = m_NextBotIdCounter++;

    // Add to list, edict is null for now, isPendingPlayerSlot = true
    m_ManagedBots.emplace_back(nullptr, currentBotId, botName, team, className);
    // BotInfo& newBot = m_ManagedBots.back(); // Not needed immediately

    // Conceptual: Tell the engine to add a bot. This is the key engine interaction.
    // Example: if (m_pEngineServer) { m_pEngineServer->CreateFakeClient(botName.c_str()); }
    std::cout << "CRCBotPlugin: Requested bot " << botName << " (ID: " << currentBotId
              << ", Team: " << team << ", Class: " << className
              << "). Waiting for engine to call ClientPutInServer." << std::endl;

    // For local testing without engine:
    // edict_t* dummyEdict = new edict_t{currentBotId}; // Create a dummy edict. LEAKS if not managed!
    // ClientPutInServer(dummyEdict, botName.c_str()); // Simulate engine callback

    return currentBotId;
}

void CRCBotPlugin::FinalizeBotAddition(BotInfo& botInfo, edict_t* pEdict) {
    botInfo.pEdict = pEdict;
    botInfo.isPendingPlayerSlot = false;
    std::cout << "CRCBotPlugin: Finalizing bot " << botInfo.name << " with edict " << pEdict << std::endl;

    // Conceptual: Resolve class name to internal game class ID
    // botInfo.classIdInternal = ResolveClassNameToGameId(botInfo.classNameRequest);

    const ClassConfigInfo* selectedClassConfig = nullptr;
    if (m_pGlobalKnowledgeBase && m_pGlobalKnowledgeBase->classConfigs) {
        for (const auto& cfg : *m_pGlobalKnowledgeBase->classConfigs) {
            if (cfg.className == botInfo.classNameRequest /* || cfg.classId == botInfo.classIdInternal */) {
                selectedClassConfig = &cfg;
                break;
            }
        }
    }
    if (!selectedClassConfig) {
         std::cout << "Warning: ClassConfig for '" << botInfo.classNameRequest << "' not found for bot " << botInfo.name << ". AI may be limited." << std::endl;
         // Potentially assign a default config or prevent AI initialization
    }

    // Conceptual: Create CFFPlayer wrapper instance for this bot
    // CFFPlayer* playerWrapper = new CFFPlayer(pEdict); // This would be owned by BotInfo
    // botInfo.playerWrapper = std::unique_ptr<CFFPlayer>(playerWrapper);

    botInfo.objectivePlanner = std::make_unique<CObjectivePlanner>(nullptr /*playerWrapper*/, m_pGlobalKnowledgeBase.get());
    botInfo.aiModule = AIFactory::CreateAIModule(
        botInfo.classNameRequest,
        nullptr /*playerWrapper*/,
        botInfo.objectivePlanner.get(),
        m_pGlobalKnowledgeBase.get(),
        selectedClassConfig
    );

    if (!botInfo.aiModule) {
        std::cout << "CRCBotPlugin: ERROR - Failed to create AI module for " << botInfo.name << std::endl;
        botInfo.isActive = false;
        return;
    }

    botInfo.isActive = true;
    std::cout << "CRCBotPlugin: Bot " << botInfo.name << " (ID: " << botInfo.botId << ") is now active." << std::endl;

    // Conceptual: Make the bot join team and class via engine commands
    // std::string teamCmd = (botInfo.teamId == 2) ? "red" : "blue";
    // EngineIssueBotCommand(pEdict, "jointeam %s", teamCmd.c_str());
    // EngineIssueBotCommand(pEdict, "joinclass %s", botInfo.classNameRequest.c_str());
}

bool CRCBotPlugin::RemoveBot(int botId) {
    auto it = std::remove_if(m_ManagedBots.begin(), m_ManagedBots.end(),
                             [botId](const BotInfo& bi) { return bi.botId == botId; });

    if (it != m_ManagedBots.end()) {
        std::cout << "CRCBotPlugin: Removing bot " << it->name << " (ID: " << botId << ")" << std::endl;
        // Conceptual: Tell engine to kick the bot
        // if (it->pEdict && m_pEngineServer) {
        //    m_pEngineServer->ServerCommand(("kickid_ex " + GetSteamIDFromEdict(it->pEdict) + " Bot removed.\n").c_str());
        // }
        // if (it->pEdict) delete it->pEdict; // If we new'd dummy edicts for testing.
        m_ManagedBots.erase(it, m_ManagedBots.end());
        return true;
    }
    std::cout << "CRCBotPlugin: RemoveBot - Bot ID " << botId << " not found." << std::endl;
    return false;
}

void CRCBotPlugin::UpdateAllBots(CUserCmd* pUserCmdArray) {
    for (BotInfo& bot : m_ManagedBots) {
        if (bot.isActive && bot.aiModule && bot.objectivePlanner && bot.pEdict /* && bot.playerWrapper->IsAlive() */) {
            CUserCmd botCmd;

            if (bot.objectivePlanner) {
                bot.objectivePlanner->EvaluateAndSelectTask();
            }

            if (bot.aiModule) {
                bot.aiModule->Update(&botCmd);
            }

            // Conceptual: Apply botCmd to the bot's entity in the game
            // Engine_ApplyUserCmd(bot.pEdict, &botCmd);
            // if (botCmd.buttons != 0) { // Log if bot is doing something
            //    std::cout << "Bot " << bot.name << " UserCmd: btns=" << botCmd.buttons
            //              << " fwd=" << botCmd.forwardmove << " side=" << botCmd.sidemove
            //              << " angX=" << botCmd.viewangles.x << std::endl;
            // }
        }
    }
}
