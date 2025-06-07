#ifndef BOT_DEFINES_H
#define BOT_DEFINES_H

// --- Control Point Task Constants ---
const float SECURE_RADIUS_CAPTURE_FF    = 250.0f;
const float SECURE_DURATION_CAPTURE_FF  = 8.0f;
const float STAND_DURATION_CAPTURE_FF   = 30.0f;
const float CP_CAPTURE_RADIUS_FF        = 90.0f;
const float SECURE_RADIUS_DEFEND_FF     = 400.0f;
const float SECURE_DURATION_DEFEND_FF   = 15.0f;
const float HOLD_DURATION_DEFEND_FF     = 60.0f;
const float CP_DEFEND_HOLD_RADIUS_FF    = 250.0f;

// --- Capture The Flag Task Constants ---
const float SECURE_RADIUS_FLAG_AREA_FF    = 350.0f;
const float SECURE_DURATION_FLAG_AREA_FF  = 10.0f;
const float HOLD_DURATION_FLAG_DEFENSE_FF = 90.0f;
const float FLAG_DEFENSE_HOLD_RADIUS_FF   = 300.0f;
const float ESCORT_FOLLOW_DISTANCE_FF     = 250.0f;
const float PICKUP_RADIUS_FF              = 75.0f;
const float PICKUP_RADIUS_SQR_FF          = PICKUP_RADIUS_FF * PICKUP_RADIUS_FF;
const float DEFAULT_FLAG_CAPTURE_RADIUS_FF  = 120.0f;
const float DEFAULT_FLAG_CAPTURE_TIMEOUT_FF = 45.0f;

// --- Engineer Specific Constants ---
#define WEAPON_NAME_PDA_FF "weapon_pda_engineer_ff_conceptual"
#define WEAPON_NAME_PDA_DEMOLISH_FF "weapon_pda_demolish_ff_conceptual"
#define WEAPON_NAME_WRENCH_FF "weapon_wrench_ff_conceptual"
#define WEAPON_NAME_SHOTGUN_ENG_FF "weapon_shotgun_engineer_ff_conceptual"
#define WEAPON_NAME_PISTOL_ENG_FF "weapon_pistol_engineer_ff_conceptual"
#define IMPULSE_BUILD_SENTRY_FF    1
#define IMPULSE_BUILD_DISPENSER_FF 2
#define IMPULSE_BUILD_TELEPORTER_ENTRANCE_FF 3
#define IMPULSE_BUILD_TELEPORTER_EXIT_FF   4
#define IMPULSE_DEMOLISH_AIMED_FF  5
#define SUBCOMMAND_PLACE_BLUEPRINT_FF 0
#define SUBCOMMAND_ROTATE_BLUEPRINT_FF 1
#define SUBCOMMAND_DEMOLISH_DIRECT_FF 2
const float WRENCH_RANGE_FF = 96.0f;
const float WRENCH_RANGE_SQR_FF = WRENCH_RANGE_FF * WRENCH_RANGE_FF;
const float WRENCH_SWING_RATE_FF = 0.8f;
const float WEAPON_SWITCH_DELAY_FF = 0.5f;
const int   WRENCH_HIT_METAL_COST_FF = 25;
const int   ENGINEER_MAX_METAL = 200;


// --- Spy Specific Constants (New) ---
#define WEAPON_NAME_REVOLVER_FF "weapon_revolver_spy_ff_conceptual"
#define WEAPON_NAME_SAPPER_FF "weapon_sapper_spy_ff_conceptual"
#define WEAPON_NAME_KNIFE_FF "weapon_knife_spy_ff_conceptual"
#define WEAPON_NAME_INVIS_WATCH_FF "weapon_invis_watch_spy_ff_conceptual"
#define WEAPON_NAME_DISGUISE_KIT_FF "weapon_disguise_kit_spy_ff_conceptual"

// Conceptual condition bits (engine specific, e.g. m_nPlayerCond)
#define COND_FF_CLOAKED         (1 << 0) // Example bit for standard cloak
#define COND_FF_DEADRINGER      (1 << 1) // Example bit for Dead Ringer cloak
#define COND_FF_DISGUISED       (1 << 2) // Example bit for being disguised
#define COND_FF_ONFIRE          (1 << 3) // Example bit for being on fire (general condition)

const float SPY_CLOAK_ABILITY_COOLDOWN_FF = 1.0f;
const float SPY_DECLOAK_TIME_FF = 0.5f;
const float SPY_DISGUISE_APPLY_TIME_FF = 0.5f;
const float SPY_SAPPER_DEPLOY_TIME_FF = 0.5f;
const float SPY_KNIFE_BACKSTAB_FOV_FF = 45.0f;
const float SPY_CLOAK_MIN_ENERGY_FF = 20.0f;      // Min energy % (0-100) to cloak
const float SAPPER_RANGE_FF = 100.0f;
const float SAPPER_RANGE_SQR_FF = SAPPER_RANGE_FF * SAPPER_RANGE_FF;
const float KNIFE_BACKSTAB_RANGE_FF = 80.0f;
const float KNIFE_BACKSTAB_RANGE_SQR_FF = KNIFE_BACKSTAB_RANGE_FF * KNIFE_BACKSTAB_RANGE_FF;
const float KNIFE_SWING_RATE_FF = 0.8f;

// Conceptual NetProp Names for Spy
#define NETPROP_SPY_DISGUISE_TEAM_FF    "m_iDisguiseTeam_FF_conceptual"
#define NETPROP_SPY_DISGUISE_CLASS_FF   "m_iDisguiseClass_FF_conceptual"
#define NETPROP_SPY_CLOAK_ENERGY_FF     "m_flCloakEnergy_FF_conceptual"
#define NETPROP_SPY_PLAYERCOND_FF       "m_nPlayerCond_FF_conceptual"  // Or m_nSharedPlayerCond

// Conceptual Class IDs (engine specific, e.g. TF_CLASS_*)
#define CLASS_SPY_FF_CONCEPTUAL 8
// ... other class IDs ...


// --- Pyro Specific Constants ---
#define WEAPON_NAME_FLAMETHROWER_FF "weapon_flamethrower_pyro_ff_conceptual"
#define WEAPON_NAME_SHOTGUN_PYRO_FF "weapon_shotgun_pyro_ff_conceptual"
#define WEAPON_NAME_AXE_PYRO_FF "weapon_axe_pyro_ff_conceptual" // Or FireAxe

#define AMMO_ID_FLAMETHROWER_FF_CONCEPTUAL 1 // Conceptual ammo ID/index for flamethrower's primary resource
#define AMMO_FLAMETHROWER_FF AMMO_ID_FLAMETHROWER_FF_CONCEPTUAL // Alias for existing usage
#define AMMO_SHOTGUN_FF 2      // Conceptual ammo type index for secondary (shared by Soldier, Engi, Heavy)
// #define AMMO_AIRBLAST_FF 20    // This was a value, not an ID. Replaced by AIRBLAST_AMMO_COST_FF_CONCEPTUAL for clarity.

const float FLAMETHROWER_EFFECTIVE_RANGE_FF = 300.0f; // Effective range for flame particles
const float FLAMETHROWER_EFFECTIVE_RANGE_SQR_FF = FLAMETHROWER_EFFECTIVE_RANGE_FF * FLAMETHROWER_EFFECTIVE_RANGE_FF;
const float FLAMETHROWER_FLAME_SPEED_FF = 1000.0f; // Conceptual speed of flame particles for prediction
const float FLAMETHROWER_FIRE_INTERVAL_FF = 0.05f; // Time between "puffs" of flame
const float SHOTGUN_EFFECTIVE_RANGE_FF = 800.0f; // Further than flames, but less effective at max range
const float SHOTGUN_EFFECTIVE_RANGE_SQR_FF = SHOTGUN_EFFECTIVE_RANGE_FF * SHOTGUN_EFFECTIVE_RANGE_FF;
const float SHOTGUN_FIRE_INTERVAL_FF = 0.625f; // Standard shotgun fire rate
const float AXE_RANGE_FF = 70.0f; // Melee range
const float AXE_RANGE_SQR_FF = AXE_RANGE_FF * AXE_RANGE_FF;
const float AXE_FIRE_INTERVAL_FF = 0.8f; // Standard melee swing rate

const float AIRBLAST_COOLDOWN_FF = 0.75f; // Cooldown between airblasts
const float AIRBLAST_RANGE_FF = 200.0f; // Effective range of airblast puff
const float AIRBLAST_FOV_DEG_FF = 60.0f; // Cone for airblasting projectiles/enemies

// Conceptual: How much ammo an airblast costs, from the flamethrower's primary ammo pool
const int AIRBLAST_AMMO_COST_FF_CONCEPTUAL = 20;
// const int AIRBLAST_AMMO_COST_FF = AIRBLAST_AMMO_COST_FF_CONCEPTUAL; // Alias if needed, but direct use of new name is better.


// Conceptual IDs
#define ENEMY_FLAG_ID_CONCEPTUAL 1
#define OUR_FLAG_ID_CONCEPTUAL   2

// --- General AI Constants ---
const float DEFAULT_ARRIVAL_TOLERANCE_FF         = 50.0f;
const float DEFAULT_ARRIVAL_TOLERANCE_SQR_FF   = DEFAULT_ARRIVAL_TOLERANCE_FF * DEFAULT_ARRIVAL_TOLERANCE_FF;
const float PATH_RECALC_TARGET_MOVED_DIST_FF   = 100.0f;
const float PATH_RECALC_TARGET_MOVED_DIST_SQR_FF = PATH_RECALC_TARGET_MOVED_DIST_FF * PATH_RECALC_TARGET_MOVED_DIST_FF;

// --- Perception System Constants ---
const float FULL_PERCEPTION_SCAN_INTERVAL_FF = 0.1f; // How often to do a full entity scan (e.g., 10 Hz)

// Conceptual Class Names (engine/game specific - for string comparisons in perception)
#define CLASSNAME_PLAYER_CONCEPTUAL "player"
#define CLASSNAME_SENTRYGUN_CONCEPTUAL "obj_sentrygun" // Example, could be "obj_sentrygun_ff"
#define CLASSNAME_DISPENSER_CONCEPTUAL "obj_dispenser"
#define CLASSNAME_TELEPORTER_CONCEPTUAL "obj_teleporter"
#define CLASSNAME_PROJECTILE_ROCKET_CONCEPTUAL "tf_projectile_rocket" // Example from TF2
#define CLASSNAME_PROJECTILE_GRENADE_CONCEPTUAL "tf_projectile_grenade" // Example from TF2
#define CLASSNAME_PROJECTILE_PIPEBOMB_CONCEPTUAL "tf_projectile_pipe" // Example for Demoman pipes
#define CLASSNAME_PROJECTILE_STICKYBOMB_CONCEPTUAL "tf_projectile_pipe_remote" // Example for stickies
// Add other relevant class names for projectiles or items if needed for perception.

// --- Team IDs ---
#define TEAM_ID_NONE      0
#define TEAM_ID_SPECTATOR 1
#define TEAM_ID_RED       2
#define TEAM_ID_BLUE      3
// Conceptual constants for callback
#define TEAM_ID_AUTO_CONCEPTUAL 0
#define TEAM_ID_RED_CONCEPTUAL 2
#define TEAM_ID_BLUE_CONCEPTUAL 3


// --- Conceptual LookAtTask types ---
enum LookAtTaskType_Conceptual {
    LOOK_AT_ENEMY, LOOK_AT_POINT, LOOK_AROUND_SCAN, LOOK_AROUND_SCAN_IN_AREA,
    LOOK_AROUND_ON_POINT, LOOK_AROUND_SCAN_DEFENSIVE
};

// --- Conceptual Disguise Team/Class Enums (if not in a more specific Spy header) ---
enum Conceptual_DisguiseTeamID { /* ... (as before) ... */ };
enum Conceptual_DisguiseClassID { /* ... (as before) ... */ };


#endif // BOT_DEFINES_H
