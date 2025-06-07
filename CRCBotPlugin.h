#ifndef RCBOT_PLUGIN_H
#define RCBOT_PLUGIN_H

#ifdef _WIN32
#pragma once
#endif

#include <vector>
#include <string>
#include <memory> // For std::unique_ptr

// Forward declarations
struct edict_t;         // Engine's entity representation (conceptual)
struct CUserCmd;        // Engine's user command structure (conceptual)
class CFFBaseAI;        // Bot's AI logic base class
class CObjectivePlanner;// Bot's high-level planning class
class CreateInterfaceFn;// Engine interface factory function pointer type
struct BotKnowledgeBase; // Forward declare (defined in FFBaseAI.h or similar)
class ClassConfigInfo;   // Forward declare (defined in ff_state_structs.h)


// Structure to hold info about managed bots
struct BotInfo {
    edict_t* pEdict;    // Conceptual: pointer to game entity, set when engine confirms spawn
    int botId;          // Unique ID for the bot
    std::string name;
    int teamId;         // Requested/Assigned Team ID
    std::string classNameRequest; // Requested Class Name
    int classIdInternal; // Game-specific class ID, resolved from classNameRequest

    std::unique_ptr<CFFBaseAI> aiModule;
    std::unique_ptr<CObjectivePlanner> objectivePlanner;

    bool isActive;      // Is this bot fully initialized and running?
    bool isPendingPlayerSlot; // True if we've asked engine for a slot, waiting for ClientPutInServer

    // Constructor
    BotInfo(edict_t* ed, int id, const std::string& botName, int team, const std::string& clsName)
        : pEdict(ed), botId(id), name(botName), teamId(team), classNameRequest(clsName),
          classIdInternal(0), // Resolve this later
          aiModule(nullptr), objectivePlanner(nullptr), isActive(false), isPendingPlayerSlot(true) {}

    // Move constructor and assignment for vector management
    BotInfo(BotInfo&& other) noexcept = default;
    BotInfo& operator=(BotInfo&& other) noexcept = default;

    // Prevent copying due to unique_ptrs
    BotInfo(const BotInfo&) = delete;
    BotInfo& operator=(const BotInfo&) = delete;
};

class CRCBotPlugin { // : public IServerPluginCallbacks { // Conceptual inheritance
public:
    CRCBotPlugin();
    virtual ~CRCBotPlugin();

    // Conceptual IServerPluginCallbacks methods
    virtual bool Load(CreateInterfaceFn interfaceFactory, CreateInterfaceFn gameServerFactory);
    virtual void Unload();
    virtual void GameFrame(bool simulating);
    virtual void LevelInit(char const *pMapName);
    virtual void LevelShutdown();
    // Callbacks for when entities are actually created/destroyed by engine
    virtual void ClientPutInServer(edict_t *pEntity, char const *playername);
    virtual void ClientDisconnect(edict_t *pEntity);


    // Bot Management
    // Returns botId if successful request, -1 otherwise
    int RequestBot(const std::string& name, int team, const std::string& className);
    bool RemoveBot(int botId); // Remove by our internal botId
    void UpdateAllBots(CUserCmd* pUserCmdArray); // pUserCmdArray is conceptual for now

private:
    void FinalizeBotAddition(BotInfo& botInfo, edict_t* pEdict); // Helper called by ClientPutInServer

    std::vector<BotInfo> m_ManagedBots; // Stores all bot instances. BotInfo.botId can be its index + 1 or a unique counter.
                                        // Using a vector means botId might not directly be index if RemoveBot compacts.
                                        // For simplicity, botId will be m_NextBotIdCounter, and we search by it.
    int m_NextBotIdCounter;             // Counter to generate unique botIds

    // Conceptual: Pointers to engine interfaces needed by the plugin itself
    // void* m_pEngineServer;
    // void* m_pGameRules; // For game mode, team scores etc.

    // Conceptual: Global knowledge, configs - loaded from Lua
    std::unique_ptr<BotKnowledgeBase> m_pGlobalKnowledgeBase;
    std::vector<ClassConfigInfo> m_GlobalClassConfigs;
};

#endif // RCBOT_PLUGIN_H
