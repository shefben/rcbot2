#ifndef FF_BASE_AI_H
#define FF_BASE_AI_H

#include <vector>
#include <string>
#include <memory> // For std::unique_ptr
#include "FFStateStructs.h" // For Vector (ensure this is created first)

// Forward Declarations
struct CUserCmd;        // Engine's user command structure (conceptual)
class CObjectivePlanner;// Bot's high-level planning class
struct SubTask;         // From BotTasks.h (conceptual, ensure defined if used directly)
class CFFPlayer;        // Conceptual wrapper for the bot's game entity
struct BotKnowledgeBase;// Shared game state, navmesh, etc.
struct ClassConfigInfo; // Bot's class-specific stats
class CBaseEntity;      // Generic game entity

class CFFBaseAI {
public:
    CFFBaseAI(CFFPlayer* pBotPlayer, CObjectivePlanner* pPlanner,
              const BotKnowledgeBase* pKnowledgeBase, const ClassConfigInfo* pClassConfig);
    virtual ~CFFBaseAI();

    // Main update called by CRCBotPlugin
    virtual void Update(CUserCmd* pCmd);

    // Target selection - pure virtual, must be implemented by derived AI classes
    virtual CBaseEntity* SelectTarget() = 0;

    // Movement
    // Initiates pathfinding to a world-space position. Stores path internally.
    // Returns true if pathfinding started, false if target is unreachable or error.
    virtual bool MoveTo(const Vector& targetPos, CUserCmd* pCmd);

    // Uses the stored path (m_CurrentPath_NavAreaIDs) to generate movement commands.
    // Returns true if still following path, false if path completed or failed to follow.
    virtual bool FollowPath(CUserCmd* pCmd);

    // Placeholder for the actual A* call, which would populate m_CurrentPath_NavAreaIDs.
    // Returns a list of nav area IDs. Empty if path not found.
    virtual std::vector<unsigned int> FindPath(const Vector& targetPos);

    // Combat - pure virtual
    virtual bool AttackTarget(CBaseEntity* pTarget, CUserCmd* pCmd) = 0;

    // Abilities - pure virtual
    virtual bool UseAbility(int abilitySlot, CBaseEntity* pTarget, CUserCmd* pCmd) = 0;

    // Subtask Execution Dispatcher - called by Update()
    // Returns true if the subtask is ongoing, false if it completed or failed in this frame.
    virtual bool ExecuteSubTask(const SubTask* pSubTask, CUserCmd* pCmd);

    // Utility Methods
    virtual bool IsTargetInRange(CBaseEntity* pTarget, float range) const;
    virtual bool IsFacingTarget(CBaseEntity* pTarget, float fovDegrees) const; // Check if bot is roughly facing the target
    virtual void AimAt(const Vector& targetPos, CUserCmd* pCmd); // Sets viewangles in pCmd
    virtual void ClearCurrentPath();

protected:
    CFFPlayer* m_pBotPlayer;
    CObjectivePlanner* m_pObjectivePlanner; // Each AI bot has its own planner instance.
    const BotKnowledgeBase* m_pKnowledgeBase; // Pointer to shared (or bot-specific) knowledge.
    const ClassConfigInfo* m_pClassConfig; // This bot's specific class configuration.

    CBaseEntity* m_pCurrentTarget; // Current combat/interaction target.
    std::vector<unsigned int> m_CurrentPath_NavAreaIDs; // Stores NavArea IDs from A*.
    int m_CurrentPathIndex; // Current index in m_CurrentPath_NavAreaIDs.
    Vector m_vCurrentMoveToTarget; // The ultimate world-space destination for the current MoveTo subtask.

    // Gets world position of the nav area ID at m_CurrentPathIndex.
    // Advances m_CurrentPathIndex if current waypoint is reached.
    // Returns true if a valid next position is available, false if path end or error.
    virtual bool GetNextPathPosition(Vector& nextPos);
};

#endif // FF_BASE_AI_H
