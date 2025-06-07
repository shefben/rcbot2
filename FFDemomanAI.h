#ifndef FF_DEMOMAN_AI_H
#define FF_DEMOMAN_AI_H

#include "FFBaseAI.h"
#include <vector> // For storing planted stickies (conceptual)

// Forward declare CUserCmd, CBaseEntity if not pulled in by FFBaseAI.h's includes
// (FFBaseAI.h should have them from its own conceptual includes or GameDefines_Placeholder.h)

// Conceptual Weapon IDs for Demoman (replace with actual game IDs)
#define WEAPON_ID_DEMOMAN_GRENADELAUNCHER   30
#define WEAPON_ID_DEMOMAN_STICKYLAUNCHER  31
#define WEAPON_ID_DEMOMAN_MELEE           32 // e.g., Bottle

#define MAX_STICKIES_FF 8 // Max active stickies Demoman can have out

class CDemomanAI_FF : public CFFBaseAI {
public:
    CDemomanAI_FF(CFFPlayer* pBotPlayer, CObjectivePlanner* pPlanner,
                  const BotKnowledgeBase* pKnowledgeBase, const ClassConfigInfo* pClassConfig);
    virtual ~CDemomanAI_FF();

    virtual CBaseEntity* SelectTarget() override;
    virtual bool AttackTarget(CBaseEntity* pTarget, CUserCmd* pCmd) override;
    // UseAbility might be used for detonation or special jumps if any
    virtual bool UseAbility(int abilitySlot, CBaseEntity* pTarget, CUserCmd* pCmd) override;

    // Demoman-specific actions that might be called by AttackTarget or by specific SubTasks
    virtual bool ActionPlantSticky(const Vector& position, CUserCmd* pCmd, bool isTrap = true, float charge = 0.0f);
    virtual bool ActionDetonateStickies(CUserCmd* pCmd);

private:
    // Conceptual tracking of planted stickies
    // struct StickyBomb_FF {
    //     void* pEntity; // Conceptual edict_t* or CBaseEntity* of the sticky bomb projectile
    //     Vector position;
    //     float plantTime;
    //     bool isTrapStickie; // Was it intended as part of a defensive trap?
    // };
    // std::vector<StickyBomb_FF> m_PlantedStickies;
    int m_iActiveStickies;          // Current count of stickies believed to be active
    float m_flNextStickyFireTime;   // Earliest time next sticky can be fired
    float m_flNextPipeFireTime;     // Earliest time next grenade can be fired

    // Helper methods for Demoman logic
    bool ShouldDetonateNow(const CBaseEntity* pPotentialTarget) const; // Check if enemies are near current stickies
    Vector FindGoodTrapLocation(const Vector& generalArea, float radius) const; // Find chokepoints or strategic spots
    Vector PredictPipeAim(const Vector& targetPos, const Vector& targetVel, float pipeSpeed) const; // For grenade launcher
    Vector PredictStickyAim(const Vector& targetPos, const Vector& targetVel, float charge) const; // For sticky launcher (direct fire)

    void SwitchToWeapon(int weaponId, CUserCmd* pCmd); // Conceptual helper (already in Soldier, could be base)
};

#endif // FF_DEMOMAN_AI_H
