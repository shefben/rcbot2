# Fortress Forever bot hooks

This document records exported bot-related functions from the Fortress Forever 2013 game and its Omni-bot bridge. These hooks are useful for integrating bots with the game server.

## Omni-Bot exported functions

The Omni-Bot library exposes the following entry points, defined in `Omni-Bot.h`:

- `BotInitialise(IEngineInterface* engine, int version)` – initialise the bot library using engine callbacks.
- `BotShutdown()` – cleanly shut down the bot library.
- `BotUpdate()` – called every frame for bot thinking.
- `BotConsoleCommand(const Arguments &args)` – handles commands issued from the server console.
- `BotAddGoal(const MapGoalDef &goaldef)` – register a map goal for bot logic.
- `BotAddBBRecord(BlackBoard_Key type, int poster, int target, obUserData* data)` – insert a blackboard record.
- `BotSendTrigger(const TriggerInfo &info)` – notify bots of triggered events.
- `BotSendEvent(int dest, const MessageHelper &msg)` – send a message to a specific bot.
- `BotSendGlobalEvent(const MessageHelper &msg)` – broadcast a message to all bots.

## Game DLL Omni-Bot stubs

`omnibot_interface.h` in the FF sources declares the stubs that the game DLL uses to talk to Omni-Bot:

- `omnibot_interface::OnDLLInit()` and `OnDLLShutdown()` – register and remove event handlers.
- `omnibot_interface::LevelInit()` – called when a new map loads.
- `omnibot_interface::InitBotInterface()` / `ShutdownBotInterface()` – load or unload the Omni-Bot library.
- `omnibot_interface::UpdateBotInterface()` – per-frame update for Omni-Bot.
- `omnibot_interface::OmnibotCommand(const CCommand &args)` – execute Omni-Bot specific console commands.
- `omnibot_interface::Trigger(CBaseEntity *ent, CBaseEntity *activator, const char *tag, const char *action)` – forward entity triggers to Omni-Bot.
- Utility helpers such as `obUtilGetBotClassFromGameClass`, `obUtilGetBotTeamFromGameTeam`, and conversions between game and bot weapon ids.
- Numerous `Notify_*` functions (e.g. `Notify_ChatMsg`, `Notify_DispenserBuilt`) inform Omni-Bot about in‑game events.

These symbols together provide a bridge between the Fortress Forever server DLL and the external Omni-Bot library so that bots can understand in‑game events and respond appropriately.

The RCBot mod loader installs basic listeners for the `player_death` and `flag_pickup` events so bots can react when a player dies or a flag is collected.

## Waypoint tools
The console command `bot_waypoint_ff` enables the Fortress Forever waypoint tool and prints a confirmation.
