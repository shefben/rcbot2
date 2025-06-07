#ifndef FF_SOLDIER_AI_H
#define FF_SOLDIER_AI_H

#include "FFBaseAI.h"

// Conceptual Weapon IDs (replace with actual FF/TF2 IDs if available)
#define WEAPON_ID_SOLDIER_ROCKETLAUNCHER 10
#define WEAPON_ID_SOLDIER_SHOTGUN      11
#define WEAPON_ID_SOLDIER_MELEE        12

class CSoldierAI_FF : public CFFBaseAI {
public:
    CSoldierAI_FF(CFFPlayer* pBotPlayer, CObjectivePlanner* pPlanner,
                  const BotKnowledgeBase* pKnowledgeBase, const ClassConfigInfo* pClassConfig);
    virtual ~CSoldierAI_FF();

    virtual CBaseEntity* SelectTarget() override;
    virtual bool AttackTarget(CBaseEntity* pTarget, CUserCmd* pCmd) override;
    virtual bool UseAbility(int abilitySlot, CBaseEntity* pTarget, CUserCmd* pCmd) override; // For rocket jump as an "ability"

    // Soldier-specific actions
    // targetOrDirection: if isDirection is true, this is a world-space unit vector.
    //                    if isDirection is false, this is a world-space target apex/position.
    virtual bool AttemptRocketJump(const Vector& targetOrDirection, bool isDirection, CUserCmd* pCmd);

private:
    // Internal state for rocket jumping
    enum class RJState {
        NONE,
        AIMING_DOWN,
        CROUCHING,
        JUMP_FIRE,
        IN_AIR
    };
    RJState m_eRocketJumpState;
    float m_flRocketJumpStateStartTime;

    void SwitchToWeapon(int weaponId, CUserCmd* pCmd); // Conceptual helper
    Vector PredictTargetPosition(CBaseEntity* pTarget, float projectileSpeed) const; // Conceptual
};

#endif // FF_SOLDIER_AI_H
