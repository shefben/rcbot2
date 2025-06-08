#ifndef FF_BASE_AI_H
#define FF_BASE_AI_H

#include <vector>
#include <string>
#include <memory> // For std::unique_ptr
#include "FFStateStructs.h"
#include "NavSystem.h" // For AStarPathfinder, NavMeshGraph, NavAreaNode

// Forward Declarations
// Assuming SDK headers will bring in CUserCmd, CBaseEntity, Vector, QAngle
// struct CUserCmd; // From game/shared/usercmd.h
// class CBaseEntity; // From game/server/cbase.h
// class Vector; // From public/mathlib/vector.h
// class QAngle; // From public/mathlib/qangle.h

class CObjectivePlanner;
struct SubTask;
class CFFPlayerWrapper; // Changed from CFFPlayer
class BotKnowledgeBase; // Changed from struct to class if it has methods
struct ClassConfigInfo;
// CBaseEntity is an SDK type, suitable for targets if we're using SDK pointers directly.
// TrackedEntityInfo* might be used by higher-level logic before calling AI methods.

class CFFBaseAI {
public:
    CFFBaseAI(CFFPlayerWrapper* pBotPlayer, CObjectivePlanner* pPlanner,
              BotKnowledgeBase* pKnowledgeBase, const ClassConfigInfo* pClassConfig); // CFFPlayerWrapper, non-const KB
    virtual ~CFFBaseAI();

    virtual void Update(CUserCmd* pCmd);
    virtual CBaseEntity* SelectTarget() = 0; // Target can be CBaseEntity* if SDK interaction is direct

    virtual bool MoveTo(const Vector& targetPos, CUserCmd* pCmd);
    virtual bool FollowPath(CUserCmd* pCmd);

    // Calls AStarPathfinder to populate m_CurrentPath_NavAreaIDs
    virtual bool PlanPathTo(const Vector& targetPos);

    virtual bool AttackTarget(CBaseEntity* pTarget, CUserCmd* pCmd) = 0;
    // UseAbility might take a target entity or a position (Vector)
    virtual bool UseAbility(int abilitySlot, CBaseEntity* pTargetEntity, const Vector& targetPosition, CUserCmd* pCmd) = 0;
    virtual bool ExecuteSubTask(const SubTask* pSubTask, CUserCmd* pCmd);

    virtual bool IsTargetInRange(CBaseEntity* pTarget, float range) const;
    virtual bool IsFacingTarget(CBaseEntity* pTarget, float fovDegrees) const;
    virtual void AimAt(const Vector& targetPos, CUserCmd* pCmd); // Uses SDK Vector/QAngle via CFFPlayerWrapper
    virtual void ClearCurrentPath();

    // SDK-aware helpers
    float GetWorldTime() const;
    bool IsLocalPlayerAlive() const; // Renamed from _SDK to avoid redundancy

    // Accessors
    CFFPlayerWrapper* GetBotPlayer() const { return m_pBotPlayer; }
    BotKnowledgeBase* GetKnowledgeBase() const { return m_pKnowledgeBase; } // Return non-const
    const ClassConfigInfo* GetClassConfig() const { return m_pClassConfig; }
    CBaseEntity* GetCurrentTarget() const { return m_pCurrentTarget; }
    void SetCurrentTarget(CBaseEntity* target) { m_pCurrentTarget = target; }


protected:
    CFFPlayerWrapper* m_pBotPlayer; // Changed type
    CObjectivePlanner* m_pObjectivePlanner;
    BotKnowledgeBase* m_pKnowledgeBase; // Changed to non-const
    const ClassConfigInfo* m_pClassConfig;

    CBaseEntity* m_pCurrentTarget; // SDK type for entity target
    std::vector<unsigned int> m_CurrentPath_NavAreaIDs;
    int m_CurrentPathIndex;
    Vector m_vCurrentMoveToTarget;

    std::unique_ptr<AStarPathfinder> m_pPathfinder; // Pathfinder instance per AI

    virtual bool GetNextPathNodePosition(Vector& nextPos); // Renamed from GetNextPathPosition
};

// Placeholder for CBotFortress if it's the concrete AI class, and AIFactory
// These would typically be in their own headers.
class CBotFortress : public CFFBaseAI { // This is a simple default AI
public:
    CBotFortress(CFFPlayerWrapper* pBotPlayer, CObjectivePlanner* pPlanner, BotKnowledgeBase* pKnowledgeBase, const ClassConfigInfo* pClassConfig)
        : CFFBaseAI(pBotPlayer, pPlanner, pKnowledgeBase, pClassConfig) {}

    CBaseEntity* SelectTarget() override { /* Basic logic or return nullptr */ return nullptr; }
    bool AttackTarget(CBaseEntity* pTarget, CUserCmd* pCmd) override { /* Basic logic or return false */ return false; }
    bool UseAbility(int abilitySlot, CBaseEntity* pTargetEntity, const Vector& targetPosition, CUserCmd* pCmd) override { return false; }
};

// Note: AIFactory::CreateAIModule should be in AIFactory.h/cpp
// For now, this is just to ensure FFBaseAI.h still compiles with the type change.
namespace AIFactory {
   inline std::unique_ptr<CFFBaseAI> CreateAIModule_PlaceholderBase( // Renamed to avoid conflict if AIFactory.h also included
        const std::string& className, CFFPlayerWrapper* pPlayer,
        CObjectivePlanner* planner, BotKnowledgeBase* kb,
        const ClassConfigInfo* classCfg)
    {
        return std::make_unique<CBotFortress>(pPlayer, planner, kb, classCfg);
    }
}

#endif // FF_BASE_AI_H
