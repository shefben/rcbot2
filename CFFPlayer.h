#ifndef CFF_PLAYER_WRAPPER_H // Renamed to avoid conflict with potential SDK CFFPlayer.h
#define CFF_PLAYER_WRAPPER_H

// SDK Includes (conceptual paths, use actual relative paths from your source directory)
#include "game/server/ff/ff_player.h" // For ::CFFPlayer (the SDK's player class)
#include "game/shared/usercmd.h"      // For CUserCmd (SDK's command structure)
#include "game/shared/in_buttons.h"   // For IN_ button flags (e.g. IN_ATTACK)
#include "public/const.h"             // For FL_ player flags (e.g. FL_ONGROUND)
// #include "game/server/weapon_base.h" // Or ff_weapon_base.h for CBaseCombatWeapon/CFFWeaponBase type if needed for active weapon checks
// #include "game/shared/ff_weapon_base.h" // For FFWeaponID enum if used in SelectWeapon
#include "game/shared/shareddefs.h"   // For TEAM_ defines (e.g. FF_TEAM_RED) and CLASS_ defines

// Bot Framework Includes
// Assuming FFStateStructs.h provided Vector and QAngle if SDK ones are not directly used or need wrapping.
// If SDK provides mathlib/vector.h and mathlib/qangle.h, those are preferred.
// For this refactor, we assume SDK types are used directly or via the includes above.
// #include "FFStateStructs.h"      // Only if still needed for custom Vector/QAngle not from SDK
#include "FFBot_SDKDefines.h"      // For our FF_BotPlayerClassID enum and mapping functions
#include "FFEngineerAI.h"          // For BuildingType_FF enum (ideally this enum is in a more general file)


// Forward declare SDK's CFFPlayer if its full definition isn't needed for these declarations
// However, since we have #include "game/server/ff/ff_player.h", it should be defined.
// class CFFPlayer; // This would refer to the SDK's CFFPlayer

class CFFPlayerWrapper { // Renamed class to avoid collision with SDK's ::CFFPlayer
public:
    CFFPlayerWrapper(edict_t* pBotEdict); // Constructor takes the bot's edict

    bool IsValid() const; // Checks if m_pSDKPlayer is valid
    bool IsAlive() const; // Uses SDK's IsAlive()

    // --- Getters using SDK ::CFFPlayer ---
    Vector GetOrigin() const;           // Returns SDK Vector
    Vector GetVelocity() const;         // Returns SDK Vector
    QAngle GetEyeAngles() const;        // Returns SDK QAngle (actual current view)
    int GetHealth() const;
    int GetMaxHealth() const;
    int GetArmor() const;
    int GetMaxArmor() const;
    int GetTeam() const;                // Returns FF_TEAM_RED, FF_TEAM_BLUE, etc. from SDK
    int GetFlags() const;               // Player FL_ flags from SDK
    bool IsDucking() const;
    bool IsOnGround() const;
    int GetAmmo(const std::string& ammoName) const; // Takes string like "AMMO_CELLS" (defined in ff_weapon_base.h)
    int GetAmmoByIndex(int ammoIndex) const;       // Takes SDK ammo index (from CAmmoDef)

    ::CFFPlayer* GetSDKPlayer() const { return m_pSDKPlayer; } // Expose SDK player pointer

    // FF-Specific State Getters
    FF_BotPlayerClassID GetBotClassId() const; // Returns our internal enum, mapped from SDK class
    std::string GetPlayerClassNameString() const; // Returns string like "soldier", "medic"
    int GetSDKClassID() const; // Returns the raw SDK CLASS_ define (e.g., CLASS_SOLDIER)
    float GetPlayerMaxSpeed() const; // From CFFPlayerClassInfo or ::CFFPlayer

    // Engineer Specific
    int GetMetalCount() const;

    // Spy Specific
    bool IsCloaked() const;         // Uses SDK ::CFFPlayer::IsCloaked()
    bool IsDisguised() const;       // Uses SDK ::CFFPlayer::IsDisguised()
    int GetDisguiseTeam() const;    // Returns SDK FF_TEAM_ define
    FF_BotPlayerClassID GetDisguiseClass() const; // Returns our internal enum, mapped from SDK disguise class
    float GetCloakEnergy() const;   // Conceptual: ::CFFPlayer may have m_flCloakEnergy or similar

    // Conceptual: Check if a specific weapon is active, e.g. Sapper
    // bool IsWeaponActive(int ff_weapon_id_enum) const;


    // --- Action Methods (filling CUserCmd or issuing ClientCommands) ---
    void SetViewAngles(CUserCmd* pCmd, const QAngle& angles); // Uses SDK QAngle
    void SetMovement(CUserCmd* pCmd, float forwardMove, float sideMove, float upMove = 0.0f);
    void AddButton(CUserCmd* pCmd, int buttonFlag);    // Uses IN_ flags from SDK
    void RemoveButton(CUserCmd* pCmd, int buttonFlag); // Uses IN_ flags from SDK
    void SetImpulse(CUserCmd* pCmd, unsigned char impulse);

    // Weapon/Tool Actions
    void SelectWeapon(CUserCmd* pCmd, const std::string& weaponClassName); // Uses "ff_weapon_..." SDK classnames
    void PrimaryAttack(CUserCmd* pCmd);   // Helper to add IN_ATTACK
    void SecondaryAttack(CUserCmd* pCmd); // Helper to add IN_ATTACK2

    // Engineer Actions (issuing server commands via plugin to the bot's edict)
    void BuildBuilding_Command(BuildingType_FF buildingType); // e.g., BuildingType_FF::SENTRY_GUN
    void DetonateBuildings_Command(); // For all engineer buildings

    // Spy Actions (issuing server commands or CUserCmd where appropriate)
    void CloakToggle_Command(); // Issues "special" client command for cloak
    void Disguise_Command(int ff_sdk_team_id, int ff_sdk_class_id); // Uses FF_TEAM_ and CLASS_ defines from SDK

private:
    edict_t*     m_pEdict;
    ::CFFPlayer* m_pSDKPlayer; // Pointer to the actual game's CFFPlayer object

    // Helper to keep m_pSDKPlayer fresh or validate it with each call or periodically
    void UpdateSDKPlayerPtr();
    // Note: Calling UpdateSDKPlayerPtr() in every getter can be expensive.
    // Consider calling it once per frame or when m_pEdict might have changed.
    // For robustness in this refactor, it might be called frequently initially.
};

#endif // CFF_PLAYER_WRAPPER_H
