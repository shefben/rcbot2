# Fortress Forever Stub Checklist

The following functions are placeholder implementations tracked via `TODO` comments.
Each item links to the source file and briefly describes what remains.

- [x] **utils/RCBot2_meta/bot_weapons_ff.cpp** `int FireFlamethrower(CBot *bot)` — implement flamethrower attack behavior.
- [x] **utils/RCBot2_meta/bot_weapons_ff.cpp** `int LaunchDetpack(CBot *bot)` — implement detpack placement and detonation.
- [x] **utils/RCBot2_meta/bot_weapons_ff.cpp** `int NailgunBurst(CBot *bot)` — implement burst logic for the nailgun.
- [x] **utils/RCBot2_meta/bot_task_ff.h** `bool CBotTaskFFMedicHeal::IsComplete() const` — detect when medic healing is finished.
- [x] **utils/RCBot2_meta/bot_task_ff.h** `bool CBotTaskFFEngineerBuild::IsComplete() const` — detect when an engineer build task is complete.
- [x] **utils/RCBot2_meta/bot_task_ff.h** `bool CBotTaskFFCaptureFlag::IsComplete() const` — determine completion once the enemy flag is captured.
- [x] **utils/RCBot2_meta/bot_task_ff.h** `bool CBotTaskFFDefendFlag::IsComplete() const` — complete when the friendly flag is secured.
- [x] **utils/RCBot2_meta/bot_ff_mod.cpp** `void CFortressForeverMod::ModeAI_CTF()` — handle Capture The Flag mode decision logic.
- [x] **utils/RCBot2_meta/bot_ff_mod.cpp** `void CFortressForeverMod::ModeAI_VIP()` — implement VIP/Hunted mode behaviors.
- [x] **utils/RCBot2_meta/bot_ff_mod.cpp** `void CFortressForeverMod::ModeAI_AD()` — implement Attack & Defend behaviors.
- [x] **utils/RCBot2_meta/bot_ff_mod.cpp** `void CFortressForeverMod::ModeAI_TC()` — implement Territorial Control behaviors.
- [x] **utils/RCBot2_meta/bot_ff_mod.cpp** `void CFortressForeverMod::ModeAI_Invade()` — implement Invade mode behaviors.
- [x] **utils/RCBot2_meta/bot_fortress.cpp** `bool CBotTF2::hasEngineerBuilt(eEngiBuild)` — verify engineer building state correctly.

