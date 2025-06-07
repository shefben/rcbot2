#ifndef CFF_PLAYER_H
#define CFF_PLAYER_H

#include "FFStateStructs.h" // For Vector (assuming QAngle might also be here or is Vector for angles)
#include <string>

// Forward declarations for engine/game types (conceptual)
struct edict_t;
struct CUserCmd;    // Represents player input
class CBaseEntity;  // Represents a generic game entity (bot could be one)

// Conceptual: If QAngle is different from Vector for angles
// struct QAngle { float x, y, z; QAngle(float _x=0, float _y=0, float _z=0) : x(_x),y(_y),z(_z){} };
// For simplicity, we'll use Vector for viewangles in CUserCmd for now if QAngle isn't in FFStateStructs.h

// Conceptual Player Flags (replace with actual game defines)
#define FL_ONGROUND (1 << 0)
#define FL_DUCKING  (1 << 1)
// ... other flags

// Conceptual Button Flags (replace with actual game defines)
#define IN_ATTACK   (1 << 0)
#define IN_JUMP     (1 << 1)
#define IN_DUCK     (1 << 2)
#define IN_RELOAD   (1 << 3)
#define IN_ATTACK2  (1 << 11) // Often used for secondary attack
// ... other buttons

class CFFPlayer {
public:
    // Constructor typically takes a pointer to the game entity this CFFPlayer instance wraps.
    // This could be an edict_t* or a CBaseEntity* depending on the engine/game architecture.
    CFFPlayer(edict_t* pEdict);
    // virtual ~CFFPlayer(); // If managing specific resources tied to this wrapper

    bool IsValid() const; // Checks if the underlying game entity is valid
    bool IsAlive() const;

    // --- Getters for Game State ---
    Vector GetOrigin() const;           // Current world position
    Vector GetVelocity() const;         // Current velocity
    Vector GetEyeAngles() const;        // Current view angles (Pitch, Yaw, Roll) - using Vector as QAngle
    int GetHealth() const;
    int GetMaxHealth() const;           // This might come from ClassConfigInfo or entity properties
    int GetArmor() const;
    int GetMaxArmor() const;            // Might come from ClassConfigInfo or entity properties
    int GetTeam() const;                // Returns team ID (e.g., 2 for RED, 3 for BLUE - game specific)
    int GetFlags() const;               // Player flags (FL_DUCKING, FL_ONGROUND etc.)

    bool IsDucking() const;
    bool IsOnGround() const;

    int GetAmmo(int ammoTypeIndex /* or string ammoName */) const; // Get ammo count for a type
    // CBaseEntity* GetActiveWeapon() const; // Conceptual: returns current weapon entity
    // std::string GetCurrentWeaponName() const; // Conceptual

    // --- Action Methods (filling UserCmd) ---
    // These methods modify a CUserCmd structure passed by reference.
    // The CUserCmd is then conceptually processed by the engine for this player's input for the frame.
    void SetViewAngles(CUserCmd* pCmd, const Vector& angles); // Using Vector as QAngle
    void SetMovement(CUserCmd* pCmd, float forwardMove, float sideMove, float upMove = 0.0f);
    void AddButton(CUserCmd* pCmd, int buttonFlag);    // e.g., IN_ATTACK
    void RemoveButton(CUserCmd* pCmd, int buttonFlag); // e.g., IN_ATTACK
    // void SelectWeapon(CUserCmd* pCmd, const std::string& weaponNameOrId); // Conceptual
    // void SelectWeaponSlot(CUserCmd* pCmd, int slot); // Conceptual
    // void ReloadWeapon(CUserCmd* pCmd); // Adds IN_RELOAD

    edict_t* GetEdict() const { return m_pEdict; }
    // CBaseEntity* GetBaseEntity() const { return m_pBaseEntity; } // If using CBaseEntity wrapper

private:
    edict_t* m_pEdict; // Pointer to the bot's game entity
    // Alternatively, or additionally:
    // CBaseEntity* m_pBaseEntity;
    // EHANDLE m_hEdict; // If using Source SDK's EHANDLE for safety against dangling pointers

    // Helper to get player property from edict or CBaseEntity (highly conceptual)
    // template<typename T> T GetPlayerProperty(const char* propName) const;
    // For example, in Source 1, this might involve datamaps or netprops.
};

#endif // CFF_PLAYER_H
