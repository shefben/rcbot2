#ifndef CMEDIC_AI_FF_H
#define CMEDIC_AI_FF_H

#include "FFBaseAI.h" // Base class
// #include "tf_weaponbase.h" // For TF2_WEAPON_MEDIGUN etc. (conceptual)
// #include "player_squad.h" // For squad logic (conceptual)

// Conceptual Weapon IDs (replace with actual FF/TF2 IDs)
#define WEAPON_ID_MEDIGUN 5
#define WEAPON_ID_SYRINGEGUN 4
#define WEAPON_ID_BONESAW 3

// Forward declaration
class CFFPlayer;
class CBaseEntity;
class UserCmd;
struct BotKnowledgeBase;
class ClassConfigInfo;
class CObjectivePlanner;


class CMedicAI_FF : public CFFBaseAI { // Or public CBotFortress if that's the pattern
public:
    CMedicAI_FF(CFFPlayer* pBotPlayer, CObjectivePlanner* pPlanner, const BotKnowledgeBase* pKnowledgeBase, const ClassConfigInfo* pClassConfig);
    virtual ~CMedicAI_FF();

    virtual void Update(UserCmd* pCmd) override; // Main update loop
    virtual CBaseEntity* SelectTarget() override; // Selects heal target or enemy

    // Overriding combat/ability as primary action is healing
    virtual bool AttackTarget(CBaseEntity* pTarget, UserCmd* pCmd) override;
    virtual bool UseAbility(int abilitySlot, CBaseEntity* pTarget, UserCmd* pCmd) override; // For UberCharge

    // Medic-specific methods
    virtual bool HealTarget(CBaseEntity* pHealTarget, UserCmd* pCmd);
    virtual bool AttemptUberCharge(UserCmd* pCmd);
    virtual CBaseEntity* FindBestHealTarget(); // Logic to find who to heal

protected:
    CBaseEntity* m_pHealTarget;
    float m_fUberChargePercentage; // 0.0 to 1.0 (or 0-100)
    bool m_bIsUberDeployed;

    // Internal state/timers
    float m_flLastHealTargetCheckTime;
    float m_flLastEnemyScanTime;

    void UpdateUberChargeLevel(); // Conceptual: updates m_fUberChargePercentage from game state
    bool ShouldDeployUber(CBaseEntity* currentEnemy) const; // Strategic condition for Uber
    void SwitchToWeapon(int weaponId, UserCmd* pCmd); // Conceptual helper
};

#endif // CMEDIC_AI_FF_H
