#ifndef FF_SPY_AI_H
#define FF_SPY_AI_H

#include "FFBaseAI.h"
#include <vector>

// Forward declare CUserCmd, CBaseEntity (typically via FFBaseAI.h's includes)
struct CUserCmd;
class CBaseEntity;
// struct BotKnowledgeBase; // From FFBaseAI.h
// struct ClassConfigInfo;  // From FFBaseAI.h

// Conceptual Enums for Spy - these would ideally be game-specific from FF defines
enum class DisguiseClassType_FF {
    NONE, SCOUT, SOLDIER, PYRO, DEMOMAN, HEAVY, ENGINEER, MEDIC, SNIPER, SPY
};
enum class DisguiseTeamType_FF {
    RED, BLUE, FRIENDLY_TEAM, ENEMY_TEAM // FRIENDLY/ENEMY relative to bot's team
};

// Conceptual Weapon IDs for Spy
#define WEAPON_ID_SPY_REVOLVER      50
#define WEAPON_ID_SPY_SAPPER        51
#define WEAPON_ID_SPY_KNIFE         52
#define WEAPON_ID_SPY_INVIS_WATCH   53 // Or a specific slot for watch
#define WEAPON_ID_SPY_DISGUISE_KIT  54 // Or an action rather than weapon

class CSpyAI_FF : public CFFBaseAI {
public:
    CSpyAI_FF(CFFPlayer* pBotPlayer, CObjectivePlanner* pPlanner,
              const BotKnowledgeBase* pKnowledgeBase, const ClassConfigInfo* pClassConfig);
    virtual ~CSpyAI_FF();

    virtual CBaseEntity* SelectTarget() override;
    virtual bool AttackTarget(CBaseEntity* pTarget, CUserCmd* pCmd) override;
    // abilitySlot: 0=Cloak/Decloak, 1=Disguise, 2=Sap (pTargetEntity is building)
    virtual bool UseAbility(int abilitySlot, CBaseEntity* pTargetEntity, CUserCmd* pCmd) override;

    // Spy-specific actions, can be called by tasks or main AI logic
    virtual bool ActionAttemptBackstab(CBaseEntity* pTarget, CUserCmd* pCmd);
    virtual bool ActionCloak(CUserCmd* pCmd);
    virtual bool ActionDecloak(CUserCmd* pCmd); // Could be same as ActionCloak if it's a toggle
    virtual bool ActionDisguise(DisguiseTeamType_FF team, DisguiseClassType_FF disguiseClass, CUserCmd* pCmd);
    virtual bool ActionSapBuilding(CBaseEntity* pBuildingEntity, CUserCmd* pCmd);

    // Conceptual state checks (would query CFFPlayer or internal state)
    bool IsCurrentlyCloaked() const;
    bool IsCurrentlyDisguised() const;
    DisguiseTeamType_FF GetCurrentDisguiseTeam() const;
    DisguiseClassType_FF GetCurrentDisguiseClass() const;
    float GetCloakEnergy() const;

private:
    // Spy-specific state (internal tracking, should be synced with actual game state via CFFPlayer)
    bool  m_bIsCurrentlyCloaked_internal;
    DisguiseTeamType_FF m_eCurrentDisguiseTeam_internal;
    DisguiseClassType_FF m_eCurrentDisguiseClass_internal;
    float m_fCloakEnergy_internal; // 0.0 to 1.0

    // Action Cooldowns / Timers
    float m_flNextCloakToggleTime;  // Time when cloak/decloak can be used again
    float m_flNextDisguiseTime;     // Time when disguise can be changed
    float m_flNextSapTime;          // Time when sapper can be placed again
    float m_flNextStabTime;         // Time when knife can swing again
    float m_flTimeSinceLastSeenByEnemy; // For cloak decision

    // Helper methods for Spy logic
    CBaseEntity* FindBestBackstabTarget() const; // Uses KB
    CBaseEntity* FindBestSappingTarget() const;  // Uses KB
    bool ShouldCloakNow() const;
    bool ShouldDecloakNow(CBaseEntity* pPotentialTarget) const;
    DisguiseClassType_FF SelectBestDisguiseClass(const BotKnowledgeBase* kb) const; // Based on enemy team composition etc.
    bool IsBehindTarget(CBaseEntity* pTarget) const; // Geometric check
    bool IsInRangeForBackstab(CBaseEntity* pTarget) const;
    bool IsInRangeForSapping(CBaseEntity* pBuilding) const;

    void SwitchToWeapon(int weaponId, CUserCmd* pCmd); // Conceptual helper
};

#endif // FF_SPY_AI_H
