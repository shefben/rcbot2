#ifndef CFF_PLAYER_H
#define CFF_PLAYER_H

#include "FFStateStructs.h" // For Vector (and QAngle if it were defined there)
#include <string>
// #include "EngineInterfaces.h" // Not strictly needed in .h if only .cpp uses globals

// Forward declarations for engine/game types (conceptual)
struct edict_t;
struct CUserCmd;    // Represents player input
// class CBaseEntity;  // Not directly stored, but methods might return it conceptually

// Conceptual: If QAngle is different from Vector for angles
// For FFStateStructs.h, Vector is used for angles.
// struct QAngle { float x, y, z; QAngle(float _x=0, float _y=0, float _z=0) : x(_x),y(_y),z(_z){} };

// Conceptual Player Flags (replace with actual game defines from e.g., const.h or player_const.h)
#ifndef FL_ONGROUND
#define FL_ONGROUND (1 << 0)
#endif
#ifndef FL_DUCKING
#define FL_DUCKING  (1 << 1)
#endif
// ... other flags

// Conceptual Button Flags (replace with actual game defines from usercmd.h or similar)
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
// ... other buttons


class CFFPlayer {
public:
    CFFPlayer(edict_t* pEdict);
    // virtual ~CFFPlayer(); // If CFFPlayer directly manages heap resources

    bool IsValid() const;
    bool IsAlive() const;

    // --- Getters for Game State ---
    Vector GetOrigin() const;
    Vector GetVelocity() const;
    Vector GetEyeAngles() const;        // Using Vector as QAngle for view angles
    int GetHealth() const;
    int GetMaxHealth() const;
    int GetArmor() const;
    int GetMaxArmor() const;
    int GetTeam() const;
    int GetFlags() const;

    bool IsDucking() const;
    bool IsOnGround() const;

    int GetAmmo(int ammoTypeIndex /* or string ammoName for some games */) const;
    // CBaseEntity* GetActiveWeaponEntity() const; // Conceptual: returns current weapon CBaseEntity
    // int GetActiveWeaponId() const; // Conceptual: returns an enum/int ID for the weapon

    // --- Action Methods (filling UserCmd) ---
    void SetViewAngles(CUserCmd* pCmd, const Vector& angles); // Using Vector as QAngle
    void SetMovement(CUserCmd* pCmd, float forwardMove, float sideMove, float upMove = 0.0f);
    void AddButton(CUserCmd* pCmd, int buttonFlag);
    void RemoveButton(CUserCmd* pCmd, int buttonFlag);
    // void SelectWeaponById(CUserCmd* pCmd, int weaponId);
    // void ReloadWeapon(CUserCmd* pCmd);

    edict_t* GetEdict() const { return m_pEdict; }
    // CBaseEntity* GetBaseEntity() const; // Helper to get CBaseEntity from m_pEdict using engine interface

private:
    edict_t* m_pEdict;
    // EHANDLE m_hEdict; // If using Source SDK's EHANDLE for safety
};

#endif // CFF_PLAYER_H
