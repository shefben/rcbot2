# RCBot2 Fortress Forever Support

This fork implements basic bot logic for the **Fortress Forever** 2013 mod. The following tables summarize the class abilities and available grenade types. The global special ability scheduler automatically triggers abilities when bots are attacking and off cooldown.

## Class Abilities

| Class   | Special Ability (cooldown) |
|---------|---------------------------|
| Scout   | Double concussion jump when on ground (10s) |
| Sniper  | Charged headshot after 1.2 s of zoom (2s) |
| Soldier | Rocket jump if health > 60 (10s) |
| Demoman | Place detpack that explodes after 5 s (15s) |
| Medic   | Überheal grants 6 s invulnerability when charge ≥100 (6s) |
| HWGuy   | Toggle assault cannon spin state (0.8s) |
| Pyro    | Afterburn burst consuming 10 fuel (5s) |
| Spy     | Toggle cloak consuming 4 cloak/sec (recharge 20s) |
| Engineer| Cycle buildables: dispenser, sentry (upgrades), jumppad (5s) |

## Grenade Types

| Type       | Effect                        |
|------------|------------------------------|
| Frag       | 120 damage in 256 hu radius   |
| Concussion | 50 damage + knockback        |
| EMP        | Drain 80 % ammo, damage per ammo |
| Incendiary | Ignite targets for 6 s        |

The grenade handling is implemented in `src/ff/weapons/ff_grenades.cpp`.

## Special Ability Scheduler

`CSpecialAbilityScheduler` registers each class on spawn and checks cooldowns every frame. When a bot's ideal task is attack and the cooldown has elapsed, `UseSpecial()` is called automatically. Scripts can pause specials via `PauseSpecials(bot, seconds)`.

