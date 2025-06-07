#ifndef FF_BASE_AI_H
#define FF_BASE_AI_H

#include <vector>
#include <string>
#include <memory> // For std::unique_ptr
#include "FFStateStructs.h"
#include "NavSystem.h" // For AStarPathfinder, NavMeshGraph, NavAreaNode

// Forward Declarations
struct CUserCmd;
class CObjectivePlanner;
struct SubTask;
class CFFPlayer;
struct BotKnowledgeBase;
struct ClassConfigInfo;
class CBaseEntity;

class CFFBaseAI {
public:
    CFFBaseAI(CFFPlayer* pBotPlayer, CObjectivePlanner* pPlanner,
              const BotKnowledgeBase* pKnowledgeBase, const ClassConfigInfo* pClassConfig);
    virtual ~CFFBaseAI();

    virtual void Update(CUserCmd* pCmd);
    virtual CBaseEntity* SelectTarget() = 0;

    virtual bool MoveTo(const Vector& targetPos, CUserCmd* pCmd);
    virtual bool FollowPath(CUserCmd* pCmd);

    // Calls AStarPathfinder to populate m_CurrentPath_NavAreaIDs
    virtual bool PlanPathTo(const Vector& targetPos);

    virtual bool AttackTarget(CBaseEntity* pTarget, CUserCmd* pCmd) = 0;
    virtual bool UseAbility(int abilitySlot, CBaseEntity* pTarget, CUserCmd* pCmd) = 0;
    virtual bool ExecuteSubTask(const SubTask* pSubTask, CUserCmd* pCmd);

    virtual bool IsTargetInRange(CBaseEntity* pTarget, float range) const;
    virtual bool IsFacingTarget(CBaseEntity* pTarget, float fovDegrees) const;
    virtual void AimAt(const Vector& targetPos, CUserCmd* pCmd);
    virtual void ClearCurrentPath();

protected:
    CFFPlayer* m_pBotPlayer;
    CObjectivePlanner* m_pObjectivePlanner;
    const BotKnowledgeBase* m_pKnowledgeBase;
    const ClassConfigInfo* m_pClassConfig;

    CBaseEntity* m_pCurrentTarget;
    std::vector<unsigned int> m_CurrentPath_NavAreaIDs;
    int m_CurrentPathIndex;
    Vector m_vCurrentMoveToTarget;

    std::unique_ptr<AStarPathfinder> m_pPathfinder; // Pathfinder instance per AI

    virtual bool GetNextPathNodePosition(Vector& nextPos); // Renamed from GetNextPathPosition
};

// Placeholder for CBotFortress if it's the concrete AI class, and AIFactory
// These would typically be in their own headers.
class CBotFortress : public CFFBaseAI {
public:
    CBotFortress(CFFPlayer* pBotPlayer, CObjectivePlanner* pPlanner, const BotKnowledgeBase* pKnowledgeBase, const ClassConfigInfo* pClassConfig)
        : CFFBaseAI(pBotPlayer, pPlanner, pKnowledgeBase, pClassConfig) {}

    CBaseEntity* SelectTarget() override { return nullptr; }
    bool AttackTarget(CBaseEntity* pTarget, CUserCmd* pCmd) override { return false; }
    bool UseAbility(int abilitySlot, CBaseEntity* pTarget, CUserCmd* pCmd) override { return false; }
};

namespace AIFactory {
   inline std::unique_ptr<CFFBaseAI> CreateAIModule(
        const std::string& className, CFFPlayer* pPlayer,
        CObjectivePlanner* planner, const BotKnowledgeBase* kb,
        const ClassConfigInfo* classCfg)
    {
        return std::make_unique<CBotFortress>(pPlayer, planner, kb, classCfg);
    }
}

#endif // FF_BASE_AI_H
