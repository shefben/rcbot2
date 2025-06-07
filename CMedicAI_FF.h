#ifndef CMEDIC_AI_FF_H
#define CMEDIC_AI_FF_H

#include "FFBaseAI.h"

// Conceptual Weapon IDs (replace with actual FF/TF2 IDs if available)
// Using different IDs than Soldier to avoid conflict in this standalone example context
#define WEAPON_ID_MEDIC_MEDIGUN 20
#define WEAPON_ID_MEDIC_SYRINGEGUN 21
#define WEAPON_ID_MEDIC_MELEE 22 // e.g., Bonesaw

// Forward declaration
class CFFPlayer;
class CBaseEntity;
struct CUserCmd;
struct BotKnowledgeBase;
struct ClassConfigInfo;
class CObjectivePlanner;


class CMedicAI_FF : public CFFBaseAI {
public:
    CMedicAI_FF(CFFPlayer* pBotPlayer, CObjectivePlanner* pPlanner,
                const BotKnowledgeBase* pKnowledgeBase, const ClassConfigInfo* pClassConfig);
    virtual ~CMedicAI_FF();

    virtual void Update(CUserCmd* pCmd) override; // Main update loop
    virtual CBaseEntity* SelectTarget() override; // Selects heal target or enemy

    // Overriding combat/ability as primary action is healing
    virtual bool AttackTarget(CBaseEntity* pTarget, CUserCmd* pCmd) override;
    virtual bool UseAbility(int abilitySlot, CBaseEntity* pTarget, CUserCmd* pCmd) override; // For UberCharge

    // Medic-specific methods
    virtual bool HealAlly(CBaseEntity* pAlly, CUserCmd* pCmd); // Changed from HealTarget for clarity
    virtual bool AttemptUberCharge(CUserCmd* pCmd);

    CBaseEntity* GetHealTarget() const { return m_pHealTarget; }

protected:
    CBaseEntity* FindBestHealTarget(); // Logic to find who to heal
    void UpdateUberChargeLevel(); // Conceptual: updates m_fUberChargePercentage from game state
    bool ShouldDeployUber(CBaseEntity* currentEnemy) const; // Strategic condition for Uber
    void SwitchToWeapon(int weaponId, CUserCmd* pCmd); // Conceptual helper

    CBaseEntity* m_pHealTarget;
    float m_fUberChargePercentage; // 0.0 to 1.0
    bool m_bIsUberDeployed;
    float m_flUberExpireTime;

    // Internal state/timers
    float m_flLastHealTargetCheckTime;
    float m_flLastEnemyScanTime; // For self-defense checks
};

#endif // CMEDIC_AI_FF_H
