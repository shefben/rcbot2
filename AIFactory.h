#ifndef AI_FACTORY_H
#define AI_FACTORY_H

#include <memory> // For std::unique_ptr
#include <string>

// Forward declarations
class CFFBaseAI;
class CFFPlayer;
class CObjectivePlanner;
struct BotKnowledgeBase;
struct ClassConfigInfo;

namespace AIFactory {

    std::unique_ptr<CFFBaseAI> CreateAIModule(
        const std::string& className,       // e.g., "soldier", "medic", "demoman"
        CFFPlayer* pBotPlayer,              // Wrapper for the bot's game entity
        CObjectivePlanner* pPlanner,        // Bot's objective planner instance
        const BotKnowledgeBase* pKB,        // Shared or bot-specific knowledge base
        const ClassConfigInfo* pClassCfg    // Configuration for the specific class
    );

} // namespace AIFactory

#endif // AI_FACTORY_H
