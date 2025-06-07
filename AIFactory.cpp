#include "AIFactory.h"
#include "FFBaseAI.h"       // For CFFBaseAI and CBotFortress (which is defined in FFBaseAI.h for now)
#include "FFSoldierAI.h"    // For CSoldierAI_FF
#include "FFMedicAI.h"      // For CMedicAI_FF
#include "FFDemomanAI.h"    // For CDemomanAI_FF (New)

// Conceptual includes for parameters, can be removed if types are fully defined through above headers
#include "FFStateStructs.h" // For ClassConfigInfo definition
// #include "CFFPlayer.h"       // Forward declared in AIFactory.h
// #include "ObjectivePlanner.h"// Forward declared in AIFactory.h
// #include "BotKnowledgeBase.h"// Forward declared in AIFactory.h


#include <iostream> // For logging if a class is not found

namespace AIFactory {

    std::unique_ptr<CFFBaseAI> CreateAIModule(
        const std::string& requestedClassName, // This can be from user input or config
        CFFPlayer* pBotPlayer,
        CObjectivePlanner* pPlanner,
        const BotKnowledgeBase* pKB,
        const ClassConfigInfo* pClassCfg)  // The actual resolved ClassConfigInfo for the bot
    {
        // Essential pointers check
        if (!pPlanner || !pKB) {
            std::cerr << "AIFactory: ERROR - Planner or KnowledgeBase is null. Cannot create AI." << std::endl;
            return nullptr;
        }
        if (!pBotPlayer) {
             std::cerr << "AIFactory: ERROR - CFFPlayer pointer is null. Cannot create AI for class: " << requestedClassName << std::endl;
            return nullptr;
        }

        std::string determinedClassName = requestedClassName;
        if (pClassCfg && !pClassCfg->className.empty()) {
            determinedClassName = pClassCfg->className;
        } else {
            std::cout << "AIFactory: Warning - pClassCfg is null or has no className. Using requestedClassName: " << requestedClassName << std::endl;
        }


        if (determinedClassName == "Soldier") {
            // std::cout << "AIFactory: Creating CSoldierAI_FF for " << determinedClassName << std::endl;
            return std::make_unique<CSoldierAI_FF>(pBotPlayer, pPlanner, pKB, pClassCfg);
        } else if (determinedClassName == "Medic") {
            // std::cout << "AIFactory: Creating CMedicAI_FF for " << determinedClassName << std::endl;
            return std::make_unique<CMedicAI_FF>(pBotPlayer, pPlanner, pKB, pClassCfg);
        } else if (determinedClassName == "Demoman" || determinedClassName == "demoman") { // Added Demoman
            // std::cout << "AIFactory: Creating CDemomanAI_FF for " << determinedClassName << std::endl;
            return std::make_unique<CDemomanAI_FF>(pBotPlayer, pPlanner, pKB, pClassCfg);
        }
        // Add other classes here:
        // else if (determinedClassName == "Scout") {
        //     return std::make_unique<CScoutAI_FF>(pBotPlayer, pPlanner, pKB, pClassCfg);
        // }
        // ...

        // Fallback to a generic CBotFortress (defined in FFBaseAI.h)
        std::cerr << "AIFactory: AI class type '" << determinedClassName << "' not specifically handled. Creating default CBotFortress AI." << std::endl;
        return std::make_unique<CBotFortress>(pBotPlayer, pPlanner, pKB, pClassCfg);
    }

} // namespace AIFactory
