#include "AIFactory.h"
#include "FFBaseAI.h"       // For CFFBaseAI and CBotFortress
#include "FFSoldierAI.h"    // For CSoldierAI_FF
#include "FFMedicAI.h"      // For CMedicAI_FF
#include "FFDemomanAI.h"    // For CDemomanAI_FF
#include "FFEngineerAI.h"   // For CEngineerAI_FF
#include "FFSpyAI.h"        // For CSpyAI_FF
#include "FFPyroAI.h"       // For CPyroAI_FF (New)

#include "FFStateStructs.h" // For ClassConfigInfo definition
#include <iostream>

namespace AIFactory {

    std::unique_ptr<CFFBaseAI> CreateAIModule(
        const std::string& requestedClassName,
        CFFPlayer* pBotPlayer,
        CObjectivePlanner* pPlanner,
        const BotKnowledgeBase* pKB,
        const ClassConfigInfo* pClassCfg)
    {
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
            return std::make_unique<CSoldierAI_FF>(pBotPlayer, pPlanner, pKB, pClassCfg);
        } else if (determinedClassName == "Medic") {
            return std::make_unique<CMedicAI_FF>(pBotPlayer, pPlanner, pKB, pClassCfg);
        } else if (determinedClassName == "Demoman" || determinedClassName == "demoman") {
            return std::make_unique<CDemomanAI_FF>(pBotPlayer, pPlanner, pKB, pClassCfg);
        } else if (determinedClassName == "Engineer" || determinedClassName == "engineer") {
            return std::make_unique<CEngineerAI_FF>(pBotPlayer, pPlanner, pKB, pClassCfg);
        } else if (determinedClassName == "Spy" || determinedClassName == "spy") {
            return std::make_unique<CSpyAI_FF>(pBotPlayer, pPlanner, pKB, pClassCfg);
        } else if (determinedClassName == "Pyro" || determinedClassName == "pyro") { // Added Pyro
            return std::make_unique<CPyroAI_FF>(pBotPlayer, pPlanner, const_cast<BotKnowledgeBase*>(pKB), pClassCfg); // Cast away const for KB if non-const in PyroAI constructor
        }
        // Add other classes here:
        // else if (determinedClassName == "Scout") {
        //     return std::make_unique<CScoutAI_FF>(pBotPlayer, pPlanner, pKB, pClassCfg);
        // }
        // ...

        std::cerr << "AIFactory: AI class type '" << determinedClassName << "' not specifically handled. Creating default CBotFortress AI." << std::endl;
        return std::make_unique<CBotFortress>(pBotPlayer, pPlanner, pKB, pClassCfg); // CBotFortress is defined in FFBaseAI.h
    }

} // namespace AIFactory
