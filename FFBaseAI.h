#ifndef FF_BASE_AI_H
#define FF_BASE_AI_H

#include "ff_state_structs.h" // For Vector, ControlPointInfo, etc.
#include "BotTasks.h"         // For SubTask, HighLevelTask
#include <vector>
#include <string>
#include <memory> // For std::unique_ptr

// Forward Declarations
class CFFPlayer;        // Represents the bot's player entity in the game
class CBaseEntity;      // Generic game entity (player, objective, etc.)
class UserCmd;          // Represents player input commands (engine specific)
class ClassConfigInfo;  // From ff_state_structs.h (though typically part of BotKnowledgeBase or CFFPlayer)
class NavMeshGraph;     // Assumed to be defined for pathfinding
class CObjectivePlanner; // Forward declaration

// Conceptual Knowledge Base Structure
struct BotKnowledgeBase {
    const NavMeshGraph* navMesh; // Pointer to the global/map navmesh data
    const std::vector<ControlPointInfo>* controlPoints; // Pointer to global CP data
    const std::vector<PayloadPathInfo>* payloadPaths;   // Pointer to global payload path data
    const std::vector<ClassConfigInfo>* classConfigs; // Pointer to global class config data
    // std::vector<EnemyInfo> visibleEnemies; // Populated by perception system
    // ObjectiveInfo currentObjective; // Game mode specific objective details

    // Pointers to avoid copying large data structures if this KB is per-bot
    // If KB is mostly shared, these could be references or const pointers.
    BotKnowledgeBase() :
        navMesh(nullptr),
        controlPoints(nullptr),
        payloadPaths(nullptr),
        classConfigs(nullptr)
        {}
};

class CFFBaseAI {
public:
    CFFBaseAI(CFFPlayer* pBotPlayer, CObjectivePlanner* pPlanner, const BotKnowledgeBase* pKnowledgeBase, const ClassConfigInfo* pClassConfig);
    virtual ~CFFBaseAI();

    // Core AI Update loop - gets subtask from planner and executes
    virtual void Update(UserCmd* pCmd);

    // Target selection - specific to derived classes
    virtual CBaseEntity* SelectTarget() = 0;

    // Movement - path is stored in m_CurrentPath by MoveTo
    // MoveTo initiates pathfinding to a position.
    virtual bool MoveTo(const Vector& targetPos, UserCmd* pCmd);
    // FollowPath generates UserCmd inputs based on m_CurrentPath.
    virtual bool FollowPath(UserCmd* pCmd);

    // Combat
    virtual bool AttackTarget(CBaseEntity* pTarget, UserCmd* pCmd);

    // Abilities
    virtual bool UseAbility(int abilitySlot, CBaseEntity* pTarget, UserCmd* pCmd); // More generic

    // Specific actions (can be called by derived classes or Update if subtask matches)
    virtual bool ExecuteSubTask(const SubTask* pSubTask, UserCmd* pCmd);


protected:
    CFFPlayer* m_pBotPlayer;
    CObjectivePlanner* m_pObjectivePlanner; // Each AI has its own planner instance
    const BotKnowledgeBase* m_pKnowledgeBase;
    const ClassConfigInfo* m_pClassConfig;

    CBaseEntity* m_pCurrentTarget;
    std::vector<unsigned int> m_CurrentPath; // Path from A* (list of nav area IDs)
    int m_CurrentPathIndex;
    Vector m_CurrentPathTargetPos; // The ultimate destination of the current m_CurrentPath

    // Utility methods
    virtual bool IsTargetInRange(CBaseEntity* pTarget, float range) const;
    virtual bool IsFacingTarget(CBaseEntity* pTarget, float fovDegrees) const;
    virtual void AimAt(const Vector& targetPos, UserCmd* pCmd);
    virtual bool GetNextPathPosition(Vector& nextPos);

    // Pathfinding call (conceptual - AStarPathfinder would be a separate class)
    virtual bool FindPath(const Vector& startPos, const Vector& endPos, std::vector<unsigned int>& outPath);
};

// Conceptual: Factory for creating AI modules (would be in FFBaseAI.cpp or a dedicated factory file)
// namespace AIFactory {
//    std::unique_ptr<CFFBaseAI> CreateAI(const std::string& className, CFFPlayer* pPlayer, CObjectivePlanner* planner, const BotKnowledgeBase* kb, const ClassConfigInfo* classCfg);
// }

// Placeholder for CBotFortress if it's the concrete AI class
class CBotFortress : public CFFBaseAI {
public:
    CBotFortress(CFFPlayer* pBotPlayer, CObjectivePlanner* pPlanner, const BotKnowledgeBase* pKnowledgeBase, const ClassConfigInfo* pClassConfig)
        : CFFBaseAI(pBotPlayer, pPlanner, pKnowledgeBase, pClassConfig) {}

    CBaseEntity* SelectTarget() override {
        // Placeholder: Basic target selection
        // if (m_pKnowledgeBase && !m_pKnowledgeBase->visibleEnemies.empty()) {
        //     return m_pKnowledgeBase->visibleEnemies[0].entity;
        // }
        return nullptr;
    }
    // Potentially override other methods like AttackTarget for FF specific weapon handling if not in base
};


namespace AIFactory {
   inline std::unique_ptr<CFFBaseAI> CreateAIModule(
        const std::string& className, // e.g., "soldier", "demoman"
        CFFPlayer* pPlayer,
        CObjectivePlanner* planner,
        const BotKnowledgeBase* kb,
        const ClassConfigInfo* classCfg)
    {
        // For now, always return a CBotFortress instance (acting as a generic combat class AI)
        // In a real system, this would switch on className or classId from classCfg
        // if (className == "soldier" || className == "demoman" etc.)
        return std::make_unique<CBotFortress>(pPlayer, planner, kb, classCfg);
        // return nullptr; // If class name not recognized
    }
}


#endif // FF_BASE_AI_H
