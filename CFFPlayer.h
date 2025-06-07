#ifndef CFF_PLAYER_H
#define CFF_PLAYER_H

#include "FFStateStructs.h"
#include <string>
// #include "FFEngineerAI.h" // Only if BuildingType_FF is needed directly by CFFPlayer methods

// Forward declarations
struct edict_t;
struct CUserCmd;
class CBaseEntity;
// enum class BuildingType_FF; // Forward declare if needed by a CFFPlayer method

// Conceptual Player Flags & Button Flags
#ifndef FL_ONGROUND
#define FL_ONGROUND (1 << 0)
#endif
#ifndef FL_DUCKING
#define FL_DUCKING  (1 << 1)
#endif
// ... (other FL_ flags if used by CFFPlayer directly) ...
#ifndef IN_ATTACK
#define IN_ATTACK   (1 << 0)
#endif
#ifndef IN_JUMP
#define IN_JUMP     (1 << 1)
#endif
#ifndef IN_DUCK
#define IN_DUCK     (1 << 2)
#endif
#ifndef IN_RELOAD
#define IN_RELOAD   (1 << 3)
#endif
#ifndef IN_ATTACK2
#define IN_ATTACK2  (1 << 11)
#endif
// ... (other IN_ flags if used by CFFPlayer directly) ...


class CFFPlayer {
public:
    CFFPlayer(edict_t* pEdict);

    bool IsValid() const;
    bool IsAlive() const;

    // --- Getters for Game State ---
    Vector GetOrigin() const;
    Vector GetVelocity() const;
    Vector GetEyeAngles() const;
    int GetHealth() const;
    int GetMaxHealth() const;
    int GetArmor() const;
    int GetMaxArmor() const;
    int GetTeam() const;
    int GetFlags() const;
    bool IsDucking() const;
    bool IsOnGround() const;
    int GetAmmo(int ammoTypeIndex) const;
    int GetActiveWeaponId_Conceptual() const;
    bool IsWeaponActive_Conceptual(const std::string& weaponName) const;

    // --- Engineer-Specific Getters ---
    int GetMetalCount_Conceptual() const;

    // --- Spy-Specific Getters (New) ---
    bool IsCloaked_Conceptual() const;
    bool IsDisguised_Conceptual() const;
    int GetDisguiseTeam_Conceptual() const;   // Returns game-specific team ID (e.g., TEAM_RED)
    int GetDisguiseClass_Conceptual() const; // Returns game-specific class ID (e.g., TF_CLASS_SOLDIER)
    float GetCloakEnergy_Conceptual() const;
    bool CanCloak_Conceptual() const;       // Checks energy, cooldowns, conditions
    bool CanDisguise_Conceptual() const;    // Checks cooldowns, conditions

    // --- Pyro Specific Getters (conceptual) ---
    bool IsOnFire_Conceptual() const;
    bool IsTargetOnFire_Conceptual(edict_t* pTargetEdict) const;
    int GetAirblastAmmo_Conceptual() const;
    bool CanAirblast_Conceptual() const;


    // --- Action Methods (filling UserCmd) ---
    void SetViewAngles(CUserCmd* pCmd, const Vector& angles);
    void SetMovement(CUserCmd* pCmd, float forwardMove, float sideMove, float upMove = 0.0f);
    void AddButton(CUserCmd* pCmd, int buttonFlag);
    void RemoveButton(CUserCmd* pCmd, int buttonFlag);
    void SelectWeaponByName_Conceptual(const std::string& weaponName, CUserCmd* pCmd);
    void SelectWeaponById_Conceptual(int weaponId, CUserCmd* pCmd);

    // --- Engineer-Specific Action Methods ---
    // void IssuePDABuildCommand_Conceptual(BuildingType_FF buildingType, int subCommand, CUserCmd* pCmd); // BuildingType_FF needs definition
    void IssuePDABuildCommand_Conceptual(int buildingTypeId, int subCommand, CUserCmd* pCmd); // Use int for type for now
    void SwingWrench_Conceptual(CUserCmd* pCmd);

    // --- Spy-Specific Action Methods (New) ---
    void StartCloak_Conceptual(CUserCmd* pCmd); // Assumes Invis Watch is active weapon, typically IN_ATTACK2
    void StopCloak_Conceptual(CUserCmd* pCmd);  // If cloak is toggled by same button or different action
    void IssueDisguiseCommand_Conceptual(int targetTeamId_conceptual, int targetClassId_conceptual); // Queues a ClientCommand
    void DeploySapper_Conceptual(CUserCmd* pCmd); // Assumes Sapper is active weapon, typically IN_ATTACK


    edict_t* GetEdict() const { return m_pEdict; }

private:
    edict_t* m_pEdict;

    // Conceptual placeholders for internal state if not directly reading from engine each call
    Vector m_CurrentPosition_placeholder;
    Vector m_CurrentViewAngles_placeholder;
    int m_iCurrentHealth_placeholder;
    int m_iCurrentMetal_placeholder;
    int m_iActiveWeaponId_placeholder;
    // Spy conceptual state placeholders (to be replaced by engine calls)
    bool m_bIsCloaked_placeholder;
    bool m_bIsDisguised_placeholder;
    int m_iDisguiseTeam_placeholder;
    int m_iDisguiseClass_placeholder;
    float m_fCloakEnergy_placeholder;
};

#endif // CFF_PLAYER_H
