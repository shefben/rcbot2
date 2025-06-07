#include "AIFactory.h"
#include "FFBaseAI.h"       // For CFFBaseAI and CBotFortress (which is defined in FFBaseAI.h for now)
#include "FFSoldierAI.h"    // For CSoldierAI_FF
#include "FFMedicAI.h"      // For CMedicAI_FF
// Include other specific AI class headers here as they are created (e.g., FFDemomanAI.h)

// Conceptual includes for parameters, can be removed if types are fully defined through above headers
// #include "CFFPlayer.h"
// #include "ObjectivePlanner.h"
// #include "BotKnowledgeBase.h"
// #include "ClassConfigInfo.h"

#include <iostream> // For logging if a class is not found

namespace AIFactory {

    std::unique_ptr<CFFBaseAI> CreateAIModule(
        const std::string& className,
        CFFPlayer* pBotPlayer,
        CObjectivePlanner* pPlanner,
        const BotKnowledgeBase* pKB,
        const ClassConfigInfo* pClassCfg)
    {
        if (!pPlanner || !pKB || !pClassCfg) {
            std::cerr << "AIFactory: ERROR - Planner, KnowledgeBase, or ClassConfig is null. Cannot create AI for class: " << className << std::endl;
            return nullptr;
        }
        // Note: pBotPlayer could be null during initial setup before entity is fully spawned,
        // but AI module would need it for most operations. For now, allow null for flexibility.

        if (className == "soldier" || (pClassCfg && pClassCfg->className == "Soldier")) { // Using pClassCfg->className for robustness
             std::cout << "AIFactory: Creating CSoldierAI_FF for " << (pClassCfg ? pClassCfg->className : className) << std::endl;
            return std::make_unique<CSoldierAI_FF>(pBotPlayer, pPlanner, pKB, pClassCfg);
        } else if (className == "medic" || (pClassCfg && pClassCfg->className == "Medic")) {
             std::cout << "AIFactory: Creating CMedicAI_FF for " << (pClassCfg ? pClassCfg->className : className) << std::endl;
            return std::make_unique<CMedicAI_FF>(pBotPlayer, pPlanner, pKB, pClassCfg);
        }
        // Add other classes here:
        // else if (className == "demoman" || (pClassCfg && pClassCfg->className == "Demoman")) {
        //     return std::make_unique<CDemomanAI_FF>(pBotPlayer, pPlanner, pKB, pClassCfg);
        // }
        // ...

        // Fallback to a generic CFFBaseAI or CBotFortress if specific class not found or as default
        std::cerr << "AIFactory: AI class '" << className << "' not specifically handled. Creating default CBotFortress AI." << std::endl;
        // CBotFortress is defined in FFBaseAI.h as a simple derivative for now.
        return std::make_unique<CBotFortress>(pBotPlayer, pPlanner, pKB, pClassCfg);
    }

} // namespace AIFactory
