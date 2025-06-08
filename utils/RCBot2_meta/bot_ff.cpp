// This file's original content has been refactored and moved.
// Core CBotFF method implementations, generic tasks/schedules,
// and Hunted Mode specific tasks/schedules are now in ff_bot/bot_ff_core.cpp
// CClassInterface implementations are in ff_bot/bot_ff_class_interface.cpp
// Class-specific logic (helper functions, tasks, schedules) is in ff_bot/classes/ (e.g., bot_ff_scout.cpp)

#include "bot_ff.h" // Still need to include the header for CBotFF declaration linkage.
// Other necessary includes that might be required by specific build system configurations,
// though ideally, this file becomes very lean or even empty if linkage is handled via headers.

// Note: The static member s_IsHuntedModeForTesting has been moved to bot_ff_core.cpp
// Note: All task and schedule implementations previously in this file have been moved.
