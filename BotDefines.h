#ifndef BOT_DEFINES_H
#define BOT_DEFINES_H

// Radii for tasks (conceptual units)
const float SECURE_RADIUS_CAPTURE_FF    = 250.0f;
const float SECURE_RADIUS_DEFEND_FF     = 400.0f;
const float CP_CAPTURE_RADIUS_FF        = 90.0f;  // How close bot needs to be to CP center to be "on" it
const float CP_DEFEND_HOLD_RADIUS_FF    = 250.0f; // How far bot can stray while "holding" a defensive position

// Durations for tasks (seconds)
const float SECURE_DURATION_CAPTURE_FF  = 8.0f;
const float SECURE_DURATION_DEFEND_FF   = 15.0f;
const float STAND_DURATION_CAPTURE_FF   = 30.0f; // Max time to attempt cap on point if not succeeding
const float HOLD_DURATION_DEFEND_FF     = 60.0f; // Hold for 60s or until new orders/threats

// General AI constants
const float DEFAULT_ARRIVAL_TOLERANCE_SQR_FF = 50.0f * 50.0f; // Squared for distance checks

// Team IDs (example, replace with actual game specific values)
#define TEAM_ID_NONE      0
#define TEAM_ID_SPECTATOR 1
#define TEAM_ID_RED       2
#define TEAM_ID_BLUE      3

// Conceptual LookAtTask types (if not defined in RCBot2's bot.h)
enum LookAtTaskType_Conceptual {
    LOOK_AT_ENEMY,
    LOOK_AT_POINT,
    LOOK_AROUND_SCAN,
    LOOK_AROUND_SCAN_IN_AREA,
    LOOK_AROUND_ON_POINT,
    LOOK_AROUND_SCAN_DEFENSIVE
};


#endif // BOT_DEFINES_H
