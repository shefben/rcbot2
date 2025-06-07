#ifndef FF_ENGINEER_AI_H
#define FF_ENGINEER_AI_H

#include "FFBaseAI.h"
#include <vector> // For potential tracking of own buildings if not solely in KB

// Forward declare CUserCmd, CBaseEntity if not pulled in by FFBaseAI.h's includes
// (FFBaseAI.h should have them from its own conceptual includes or GameDefines_Placeholder.h)
struct CUserCmd;
class CBaseEntity;
// struct BotKnowledgeBase; // Already included via FFBaseAI.h
// struct ClassConfigInfo;  // Already included via FFBaseAI.h


// Conceptual enum for building types. Specific to Fortress Forever.
// These values might map to integers used in game commands (e.g., `build 0` for sentry).
enum class BuildingType_FF {
    SENTRY_GUN,
    DISPENSER,
    TELEPORTER_ENTRANCE,
    TELEPORTER_EXIT,
    // Add FF-specific buildings if any (e.g., MiniSentry, different levels)
    SENTRY_GUN_LEVEL1, SENTRY_GUN_LEVEL2, SENTRY_GUN_LEVEL3, // Example for levels
    DISPENSER_LEVEL1, DISPENSER_LEVEL2, DISPENSER_LEVEL3,
    UNKNOWN
};

// Conceptual Weapon IDs for Engineer (replace with actual game IDs)
#define WEAPON_ID_ENGINEER_SHOTGUN    40
#define WEAPON_ID_ENGINEER_PISTOL     41 // Or wrangler if it's a weapon
#define WEAPON_ID_ENGINEER_WRENCH     42
#define WEAPON_ID_ENGINEER_PDA_BUILD  43 // PDA for building
#define WEAPON_ID_ENGINEER_PDA_DEMOLISH 44 // PDA for demolishing (or same PDA, different mode)

// Conceptual metal cost for buildings
#define METAL_COST_SENTRY_FF    130
#define METAL_COST_DISPENSER_FF 100
#define METAL_COST_TELEPORTER_ENTRANCE_FF 50 // TF2 costs, FF may vary
#define METAL_COST_TELEPORTER_EXIT_FF   50

class CEngineerAI_FF : public CFFBaseAI {
public:
    CEngineerAI_FF(CFFPlayer* pBotPlayer, CObjectivePlanner* pPlanner,
                   const BotKnowledgeBase* pKnowledgeBase, const ClassConfigInfo* pClassConfig);
    virtual ~CEngineerAI_FF();

    virtual CBaseEntity* SelectTarget() override;
    virtual bool AttackTarget(CBaseEntity* pTarget, CUserCmd* pCmd) override;
    // UseAbility might dispatch to build/repair/upgrade/demolish actions
    // abilitySlot could indicate BuildingType for build, or a specific action (0=repair, 1=upgrade, 2=demolish)
    // pTargetOrPos could be a CBaseEntity* (for repair/upgrade) or a Vector* (for build position)
    virtual bool UseAbility(int abilitySlot, CBaseEntity* pTargetEntity, CUserCmd* pCmd) override;
    // Overload for position based abilities if needed, or pack Vector into a conceptual CBaseEntity wrapper
    virtual bool UseAbilityAtPosition(int abilitySlot, const Vector& targetPos, CUserCmd* pCmd);


    // Engineer-specific actions, often triggered by tasks or planner decisions
    virtual bool ActionBuild(BuildingType_FF type, const Vector& designatedBuildPos, CUserCmd* pCmd);
    virtual bool ActionRepair(CBaseEntity* pBuildingEntity, CUserCmd* pCmd);
    virtual bool ActionUpgrade(CBaseEntity* pBuildingEntity, CUserCmd* pCmd); // Hits with wrench to upgrade
    virtual bool ActionWrenchHit(CBaseEntity* pTargetEntity, CUserCmd* pCmd); // For construction, repair, upgrade, or combat
    virtual bool ActionDetonateAllBuildings(CUserCmd* pCmd); // Conceptual "demolish all"

    // State / Helpers
    CBaseEntity* GetSentry() const;
    CBaseEntity* GetDispenser() const;
    CBaseEntity* GetTeleporterEntrance() const;
    CBaseEntity* GetTeleporterExit() const;
    int GetMetalCount() const;

private:
    // Conceptual state for current build goal or active buildings
    struct EngineerBuildingInfo {
        BuildingType_FF type;
        CBaseEntity* pEntity; // Pointer to the actual building entity
        int level;
        int health;
        int maxHealth;
        bool isBuilding; // Is it currently a blueprint being constructed?
        bool isUpgrading;
        Vector position;

        EngineerBuildingInfo() : type(BuildingType_FF::UNKNOWN), pEntity(nullptr), level(0), health(0), maxHealth(0), isBuilding(false), isUpgrading(false) {}
    };

    EngineerBuildingInfo m_SentryInfo;
    EngineerBuildingInfo m_DispenserInfo;
    EngineerBuildingInfo m_TeleporterEntranceInfo;
    EngineerBuildingInfo m_TeleporterExitInfo;

    BuildingType_FF m_eNextBuildingToPlace; // If cycling through build order
    Vector m_vCurrentHaulTargetPos;       // If hauling a building
    CBaseEntity* m_pBuildingToHaul;       // If hauling

    float m_fNextWrenchHitTime;
    float m_fNextBuildCmdTime; // Cooldown for issuing build commands via PDA

    // Helper methods for Engineer logic
    void UpdateBuildingStates(const BotKnowledgeBase* kb); // Update info about own buildings
    Vector FindBestBuildLocation(BuildingType_FF type, const Vector& nearLocationHint, const BotKnowledgeBase* kb) const;
    bool NeedsToBuild(BuildingType_FF type, const BotKnowledgeBase* kb) const; // Checks if a building type is missing or destroyed
    CBaseEntity* FindBestBuildingToMaintain() const; // Finds damaged or upgradable building
    bool SelectPDABuild(BuildingType_FF type, CUserCmd* pCmd); // Selects building on PDA

    void SwitchToWeapon(int weaponId, CUserCmd* pCmd); // Conceptual (already in Soldier, could be base)
    int GetBuildingCost(BuildingType_FF type) const;
};

#endif // FF_ENGINEER_AI_H
