#ifndef CMEDIC_AI_FF_H
#define CMEDIC_AI_FF_H

#include "FFBaseAI.h"

// Conceptual Weapon IDs (replace with actual FF/TF2 IDs if available)
// Using different IDs than Soldier to avoid conflict in this standalone example context
#define WEAPON_ID_MEDIC_MEDIGUN 20
#define WEAPON_ID_MEDIC_SYRINGEGUN 21
#define WEAPON_ID_MEDIC_MELEE 22 // e.g., Bonesaw

// Forward declaration
class CFFPlayerWrapper; // Updated
class CBaseEntity;      // SDK Type
struct CUserCmd;        // SDK Type
class BotKnowledgeBase; // Updated
struct ClassConfigInfo;
class CObjectivePlanner;
struct TrackedEntityInfo; // For m_pHealTargetInfo


class CMedicAI_FF : public CFFBaseAI {
public:
    CMedicAI_FF(CFFPlayerWrapper* pBotPlayer, CObjectivePlanner* pPlanner,
                BotKnowledgeBase* pKnowledgeBase, const ClassConfigInfo* pClassConfig); // Updated params
    virtual ~CMedicAI_FF();

    virtual void Update(CUserCmd* pCmd) override;
    virtual CBaseEntity* SelectTarget() override;

    virtual bool AttackTarget(CBaseEntity* pTarget, CUserCmd* pCmd) override;
    virtual bool UseAbility(int abilitySlot, CBaseEntity* pTargetEntity, const Vector& targetPosition, CUserCmd* pCmd) override; // Signature matches CFFBaseAI

    // Medic-specific methods
    virtual bool HealAlly(CBaseEntity* pAlly, CUserCmd* pCmd);
    virtual bool AttemptUberCharge(CUserCmd* pCmd);

    TrackedEntityInfo* GetHealTargetInfo() const { return m_pHealTargetInfo; } // Changed getter

protected:
    TrackedEntityInfo* FindBestHealTarget(); // Return TrackedEntityInfo*
    void UpdateUberChargeLevel();
    bool ShouldDeployUber(CBaseEntity* currentEnemyContext) const; // currentEnemyContext from GetCurrentTarget()
    void SwitchToWeapon(const std::string& weaponClassName, CUserCmd* pCmd); // Takes classname string

    TrackedEntityInfo* m_pHealTargetInfo; // Changed type
    float m_fUberChargePercentage;
    bool m_bIsUberDeployed;
    float m_flUberExpireTime;

    // Internal state/timers
    float m_flLastHealTargetCheckTime;
    float m_flLastEnemyScanTime; // For self-defense checks
};

#endif // CMEDIC_AI_FF_H
