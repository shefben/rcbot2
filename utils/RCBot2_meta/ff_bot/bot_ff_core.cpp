// bot_ff_core.cpp
// Core implementations for CBotFF methods and generic FF tasks/schedules.

#include "../bot_ff.h"
#include "../bot_globals.h"
#include "../bot_navigation.h"
#include "../bot_manager.h"
#include "../bot_event.h"
#include "../bot_util.h"
#include "../bot_math.h"
#include "../bot_waypoint.h"
#include "../bot_task.h"
#include "../bot_schedule.h"
#include "../bot_weapon_defs.h"
#include "../bot_projectiles.h" // For m_NearestEnemyRocket if used in moved logic
#include "../bot_game.h"      // For BotSupport

#include "game_ff.h" // For FF specific game constants / enums if this exists
#include "game_shared/ff/ff_playerclass_parse.h" // For CFFPlayerClassInfo
#include "game_shared/ff/ff_shareddefs.h" // For FF_CLASS_ enums
#include "game_shared/ff/ff_player.h" // For CFFPlayer if direct member access is used

// Core CBotFF method implementations and generic tasks/schedules will be moved here.
bool CBotFF::s_IsHuntedModeForTesting = false; // Definition for static member
