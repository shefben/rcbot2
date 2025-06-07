#ifndef FFPYROAI_H
#define FFPYROAI_H

#include "FFBaseAI.h"

// Forward declarations (conceptual, actual types from SDK or GameDefines_Placeholder.h)
// struct CUserCmd; // Already in GameDefines_Placeholder.h, included via FFBaseAI.h -> BotKnowledgeBase.h
// class CBaseEntity; // Conceptual base entity class, FFBaseAI.h uses TrackedEntityInfo for targets
class TrackedEntityInfo; // Used by CFFBaseAI for targets

class CPyroAI_FF : public CFFBaseAI {
public:
    CPyroAI_FF(CFFPlayer* pBotPlayer, CObjectivePlanner* pPlanner,
               BotKnowledgeBase* pKnowledgeBase, const ClassConfigInfo* pClassConfig); // Changed const KB to non-const

    virtual TrackedEntityInfo* SelectTarget() override; // Changed CBaseEntity* to TrackedEntityInfo*
    virtual bool AttackTarget(TrackedEntityInfo* pTarget, CUserCmd* pCmd) override; // Changed CBaseEntity* to TrackedEntityInfo*
    virtual bool UseAbility(int abilitySlot, TrackedEntityInfo* pTargetEntity, const Vector& targetOrDirection, CUserCmd* pCmd) override; // Modified signature

    // Pyro-specific actions
    // Changed optionalTargetPos to required, as airblast always has a direction.
    // If no specific target, it could be player's forward view direction.
    virtual bool ActionAirblast(CUserCmd* pCmd, const Vector& aimTargetPos);

    // Conceptual state checks / helpers
    // These would typically take TrackedEntityInfo or entity IDs and use BotKnowledgeBase
    bool IsTargetOnFire(const TrackedEntityInfo* pTarget) const;
    TrackedEntityInfo* FindNearbyBurningAlly() const;
    TrackedEntityInfo* FindReflectableProjectile() const;
    bool ShouldSpyCheck(const TrackedEntityInfo* pTeammate) const;

private:
    float m_fNextFlamethrowerTime;
    float m_fNextShotgunTime;
    float m_fNextAxeTime;
    float m_fNextAirblastTime;

    // Internal helper for aiming
    // void AimAtTarget(TrackedEntityInfo* pTarget, CUserCmd* pCmd, bool bPredict = false, float fProjectileSpeed = 0.0f);
    // void AimAtPosition(const Vector& targetPos, CUserCmd* pCmd);
};

#endif // FFPYROAI_H
