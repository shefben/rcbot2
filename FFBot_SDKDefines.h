#ifndef FFBOT_SDKDEFINES_H
#define FFBOT_SDKDEFINES_H

#include <string> // For std::string in mapping functions

// Assuming SDK mathlib will provide Vector and QAngle via other includes like public/edict.h or specific math headers.
// Assuming SDK const.h / input.h / buttons.h will provide FL_ and IN_ flags (e.g. included via player_command.h or cbase.h).
// Assuming SDK game/shared/shareddefs.h provides CLASS_* and TEAM_* defines (e.g. included via ff_player.h).

// Internal Bot Player Class IDs
// These values should mirror the CLASS_XXX defines from Fortress Forever's shareddefs.h for easy mapping.
// Example: if FF's CLASS_SCOUT is 1, then FF_BOT_CLASS_SCOUT should be 1.
enum FF_BotPlayerClassID {
    FF_BOT_CLASS_UNKNOWN = 0,    // Typically matches CLASS_NONE or CLASS_CIVILIAN if 0 isn't used
    FF_BOT_CLASS_SCOUT = 1,
    FF_BOT_CLASS_SNIPER = 2,
    FF_BOT_CLASS_SOLDIER = 3,
    FF_BOT_CLASS_DEMOMAN = 4,
    FF_BOT_CLASS_MEDIC = 5,
    FF_BOT_CLASS_HVYWEAP = 6,    // Often referred to as HWGuy in FF
    FF_BOT_CLASS_PYRO = 7,
    FF_BOT_CLASS_SPY = 8,
    FF_BOT_CLASS_ENGINEER = 9,
    FF_BOT_CLASS_CIVILIAN = 10,  // If distinct from UNKNOWN and has specific logic
    FF_BOT_CLASS_MAX           // For iteration or array sizing, should be one greater than the last valid class
};

// Mapping functions (declarations)
// Implementations would typically go into a utility .cpp file (e.g., FFBotUtils.cpp)
FF_BotPlayerClassID GetBotClassIDFromString_FF(const std::string& className);
// FF_BotPlayerClassID GetBotClassIDFromSlot_FF(int ff_sdk_slot_id);
int GetSDKClassIDFromBotEnum_FF(FF_BotPlayerClassID botClassId);
const char* GetPlayerClassNameFromBotID_FF(FF_BotPlayerClassID botClassId);
std::string GetPlayerClassShortNameFromBotID_FF(FF_BotPlayerClassID botClassId);

// Weapon and Ammo Mapping Functions (declarations)
// These map between string names (e.g. "ff_weapon_rocketlauncher", "AMMO_ROCKETS")
// and SDK integer IDs/indices (e.g. FFWeaponID enum, ammo definition index).
int GetSDKWeaponIDFromString_FF(const std::string& weaponName); // Returns FFWeaponID or similar SDK enum/int
const char* GetWeaponStringFromSDKID_FF(int weaponId);       // For debugging or logging
int GetAmmoIndexForName_FF(const std::string& ammoName);     // Returns int index for CBaseCombatCharacter::GetAmmoCount()

// Team IDs (for reference, these should come from FF SDK's shareddefs.h or similar)
// Example: FF_TEAM_RED, FF_TEAM_BLUE from ff_shareddefs.h or player_resource.h
// We define our own for consistency if needed, but ensure values match SDK.
// Using our own TEAM_ID_AUTO_FF for bot requests is fine.
#define TEAM_ID_BOT_AUTO_ASSIGN 0 // Special value for auto-assign, might map to SDK's TEAM_UNASSIGNED or a specific auto-assign logic

// Fortress Forever Specific Team Defines (Conceptual, actual values from SDK's shareddefs.h)
// #define FF_TEAM_NONE            0 // Typically TEAM_UNASSIGNED in Source SDK
// #define FF_TEAM_SPECTATOR       1 // Typically TEAM_SPECTATOR
// #define FF_TEAM_RED             2
// #define FF_TEAM_BLUE            3
// #define FF_TEAM_GREEN           4 // If FF supports more teams
// #define FF_TEAM_YELLOW          5

// Bot-specific defines (not directly mirroring SDK types but for our plugin's logic)

// FCVAR flags for ConCommands/ConVars created by the plugin.
// These FCVAR_ define values (e.g., FCVAR_GAMEDLL, FCVAR_CHEAT) will come from the SDK's tier1/iconvar.h (or similar).
// We can alias them if we want a plugin-specific prefix or ensure consistency.
#define FCVAR_PLUGIN_CONCOMMAND_FLAGS (FCVAR_GAMEDLL | FCVAR_CHEAT | FCVAR_SERVER_CAN_EXECUTE) // Example combination


// --- Fortress Forever Specific Entity Classnames (Examples - verify from FF SDK) ---
const char* const FF_CLASSNAME_PLAYER = "player";
const char* const FF_CLASSNAME_SENTRY = "obj_sentrygun"; // Or specific to FF like "ff_obj_sentrygun"
const char* const FF_CLASSNAME_DISPENSER = "obj_dispenser";
const char* const FF_CLASSNAME_TELEPORTER = "obj_teleporter"; // Entrance and Exit might have different classnames or be distinguished by props
const char* const FF_CLASSNAME_ROCKET_PROJ = "tf_projectile_rocket"; // Example, verify FF's actual rocket projectile classname
const char* const FF_CLASSNAME_GRENADE_PROJ = "tf_projectile_grenade"; // Example
const char* const FF_CLASSNAME_PIPEBOMB_PROJ = "tf_projectile_pipe";   // Example
const char* const FF_CLASSNAME_STICKYBOMB_PROJ = "tf_projectile_pipe_remote"; // Example
// Add more as needed (e.g. flag entity classname "item_teamflag")


// --- Fortress Forever Specific Game Event Names (Examples - verify from FF SDK) ---
const char* const FF_EVENT_POINT_CAPTURED = "ff_point_captured"; // Or "teamplay_point_captured", "controlpoint_captured"
const char* const FF_EVENT_FLAG_CAPTURED = "ff_flag_captured";   // Or "teamplay_flag_event" with type subfield
const char* const FF_EVENT_PLAYER_DEATH = "player_death";
const char* const FF_EVENT_PLAYER_SPAWN = "player_spawn";
const char* const FF_EVENT_BUILDING_PLACED = "ff_building_placed"; // e.g. "object_placed"
const char* const FF_EVENT_BUILDING_DESTROYED = "ff_building_destroyed"; // e.g. "object_destroyed"
// Add more as needed


// --- Fortress Forever Specific NetProps / Datamaps (Examples - verify from FF SDK) ---
// These would be used if direct entity property access is chosen over getter methods.
// const char* FF_NETPROP_PLAYER_HEALTH = "m_iHealth";
// const char* FF_NETPROP_PLAYER_TEAM = "m_iTeamNum";
// const char* FF_NETPROP_PLAYER_FLAGS = "m_fFlags";
// const char* FF_NETPROP_BUILDING_LEVEL = "m_iUpgradeLevel";
// const char* FF_NETPROP_BUILDING_SAPPED = "m_bDisabled"; // Or "m_bSapped"


// --- Nav Mesh Attribute Mappings (Conceptual - verify from FF SDK's nav_mesh.h or similar) ---
// Assuming our internal NavAttribute enum is defined in FFStateStructs.h or NavSystem.h
// And SDK has defines like NAV_MESH_CROUCH, NAV_MESH_JUMP etc.
// This section helps map SDK attributes to our internal bot's understanding if they differ.
// Example:
// #define NAV_ATTR_CROUCH_SDK NAV_MESH_CROUCH  // If SDK define is NAV_MESH_CROUCH
// #define NAV_ATTR_JUMP_SDK   NAV_MESH_JUMP    // If SDK define is NAV_MESH_JUMP
// #define NAV_ATTR_DANGER_SDK NAV_MESH_DANGER  // If SDK define is NAV_MESH_DANGER
// If our internal enum NavAttribute::CROUCH_BOTDEF = 1, NavAttribute::JUMP_BOTDEF = 2, etc.
// and SDK NAV_MESH_CROUCH = (1<<1), NAV_MESH_JUMP = (1<<2), the mapping in LoadNavMesh becomes crucial.
// For now, assume direct mapping or that mapping logic is handled in LoadNavMesh directly.


#endif // FFBOT_SDKDEFINES_H
