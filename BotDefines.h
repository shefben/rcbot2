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
const float PICKUP_RADIUS_FF              = 75.0f; // How close bot needs to be to flag to pick up
const float PICKUP_RADIUS_SQR_FF          = PICKUP_RADIUS_FF * PICKUP_RADIUS_FF;
const float DEFAULT_FLAG_CAPTURE_RADIUS_FF  = 120.0f; // Radius of the capture zone
const float DEFAULT_FLAG_CAPTURE_TIMEOUT_FF = 45.0f; // Max time to spend trying to cap after arriving

// Conceptual IDs (these would be game-specific, possibly dynamic)
#define ENEMY_FLAG_ID_CONCEPTUAL 1
#define OUR_FLAG_ID_CONCEPTUAL   2


// --- General AI Constants ---
const float DEFAULT_ARRIVAL_TOLERANCE_FF         = 50.0f;
const float DEFAULT_ARRIVAL_TOLERANCE_SQR_FF   = DEFAULT_ARRIVAL_TOLERANCE_FF * DEFAULT_ARRIVAL_TOLERANCE_FF;
const float PATH_RECALC_TARGET_MOVED_DIST_FF   = 100.0f; // If target for MoveToEntity moves this far, replan
const float PATH_RECALC_TARGET_MOVED_DIST_SQR_FF = PATH_RECALC_TARGET_MOVED_DIST_FF * PATH_RECALC_TARGET_MOVED_DIST_FF;


// --- Team IDs (Example, replace with actual game specific values) ---
#define TEAM_ID_NONE      0
#define TEAM_ID_SPECTATOR 1
#define TEAM_ID_RED       2
#define TEAM_ID_BLUE      3

// --- Conceptual LookAtTask types (if not defined in RCBot2's bot.h) ---
enum LookAtTaskType_Conceptual { /* ... (as before) ... */ };


#endif // BOT_DEFINES_H
