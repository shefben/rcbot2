import random

FF_TEAM_BLUE = 2
FF_TEAM_RED = 3
FF_TEAM_YELLOW = 4
FF_TEAM_GREEN = 5

# list of playable teams for random selection
FF_TEAMS = [FF_TEAM_BLUE, FF_TEAM_RED, FF_TEAM_YELLOW, FF_TEAM_GREEN]

# Fortress Forever class identifiers
FF_CLASS_SCOUT = 1
FF_CLASS_SNIPER = 2
FF_CLASS_SOLDIER = 3
FF_CLASS_DEMOMAN = 4
FF_CLASS_MEDIC = 5
FF_CLASS_HWGUY = 6
FF_CLASS_PYRO = 7
FF_CLASS_SPY = 8
FF_CLASS_ENGINEER = 9
FF_CLASS_CIVILIAN = 10

FF_CLASSES = list(range(FF_CLASS_SCOUT, FF_CLASS_CIVILIAN + 1))

def random_team() -> int:
    """Return a random valid FF team."""
    return random.choice(FF_TEAMS)

def random_class() -> int:
    """Return a random valid FF class."""
    return random.choice(FF_CLASSES)

# Voice command identifiers
FF_VC_INCOMING = 0
FF_VC_NEED_DISPENSER = 1

# Voice menu string to enum mapping
VOICE_CMD_MAP = {
    "incoming!": FF_VC_INCOMING,
    "need dispenser": FF_VC_NEED_DISPENSER,
}

FF_WEAPONS = {
    0: "none",
    1: "crowbar",
    2: "knife",
    3: "medkit",
    4: "spanner",
    5: "umbrella",
    6: "shotgun",
    7: "supershotgun",
    8: "nailgun",
    9: "supernailgun",
    10: "grenadelauncher",
    11: "pipelauncher",
    12: "autorifle",
    13: "sniperrifle",
    14: "flamethrower",
    15: "incendiarycannon",
    16: "railgun",
    17: "jumpgun",
    18: "tranq",
    19: "assaultcannon",
    20: "rpg",
    21: "tommygun",
    22: "cubemap",
    23: "deploydispenser",
    24: "deploysentrygun",
    25: "deploydetpack",
    26: "deploymancannon",
    27: "deployjumppad",
    28: "flag",
}

def ff_weapon_name(idx: int) -> str:
    return FF_WEAPONS.get(idx, "unknown")


def parse_ff_class_loadout(info):
    """Simplified parser returning weapon short name and validated team."""
    weapon_name = ff_weapon_name(info.get("weapon", 0))
    team = info.get("team")
    if team not in (FF_TEAM_BLUE, FF_TEAM_RED, FF_TEAM_YELLOW, FF_TEAM_GREEN):
        team = FF_TEAM_BLUE
    return {"weapon": weapon_name, "team": team}

# Voice command processor
def process_voice_command(text: str):
    """Convert a voice menu string into an enum value and print a debug line."""
    idx = VOICE_CMD_MAP.get(text.lower())
    if idx is not None:
        print(f"Voice cmd {idx} -> {text}")
    return idx

FF_MODE_CTF = 0
FF_MODE_VIP = 1
FF_MODE_AD = 2
FF_MODE_TC = 3
FF_MODE_INVADE = 4

def detect_ff_map_mode(mapname: str) -> int:
    name = mapname.lower()
    if 'hunted' in name:
        return FF_MODE_VIP
    if 'dustbowl' in name or '_ad' in name:
        return FF_MODE_AD
    if 'cz2' in name:
        return FF_MODE_TC
    if 'invade' in name:
        return FF_MODE_INVADE
    return FF_MODE_CTF

# Basic squad assignment helper
def assign_squads_ff(bots):
    """Split bots into offense and defense squads."""
    offense = []
    defense = []
    for bot in bots:
        if len(offense) < 3:
            offense.append(bot)
        else:
            defense.append(bot)
    print(f"Offense squad: {offense}")
    print(f"Defense squad: {defense}")
    return offense, defense

class FFScout:
    def __init__(self):
        self.conc = 2
        self.frag = 2
        self.next_special_use = 0.0
        self.grenade_primed = False
        self.on_ground = True

    def use_special(self, now: float) -> bool:
        if self.conc <= 0 or not self.on_ground or now < self.next_special_use:
            return False
        self.conc -= 1
        self.grenade_primed = True
        self.next_special_use = now + 10.0
        return True


class FFMedic:
    def __init__(self):
        self.charge = 0.0
        self.next_special_use = 0.0

    def think_heal(self):
        if self.charge < 100.0:
            self.charge += 1.0

    def use_special(self, now: float) -> bool:
        if now < self.next_special_use or self.charge < 100.0:
            return False
        self.charge = 0.0
        self.next_special_use = now + 6.0
        return True


class FFHWGuy:
    def __init__(self):
        self.spinning = False
        self.next_special_use = 0.0

    def toggle_spin(self, now: float) -> bool:
        if now < self.next_special_use:
            return False
        self.spinning = not self.spinning
        self.next_special_use = now + 0.8
        return True


BOT_TASK_NONE = 0
BOT_TASK_ATTACK = 1

class SpecialAbilityScheduler:
    def __init__(self):
        self.entries = {}

    def register(self, cls, cooldown: float):
        self.entries[cls] = {
            'cooldown': cooldown,
            'next': 0.0,
            'paused_until': 0.0,
        }

    def pause_specials(self, cls, seconds: float, now: float):
        if cls in self.entries:
            self.entries[cls]['paused_until'] = now + seconds

    def bot_frame(self, now: float):
        for cls, data in self.entries.items():
            if now < data['paused_until']:
                continue
            task = getattr(cls, 'get_ideal_task', lambda: BOT_TASK_NONE)()
            if task == BOT_TASK_ATTACK and now >= data['next']:
                if cls.use_special(now):
                    data['next'] = now + data['cooldown']

SPECIAL_SCHEDULER = SpecialAbilityScheduler()


class BuildingManager:
    def __init__(self):
        self.sentry_level = 0

    def build_dispenser(self):
        pass

    def build_sentry(self):
        if self.sentry_level < 3:
            self.sentry_level += 1
        return self.sentry_level

    def build_jumppad(self):
        pass


class FFEngineer:
    def __init__(self, mgr: BuildingManager):
        self.build_cycle = 0
        self.mgr = mgr

    def use_special(self, now: float) -> bool:
        if self.build_cycle == 0:
            self.mgr.build_dispenser()
        elif self.build_cycle == 1:
            self.mgr.build_sentry()
        else:
            self.mgr.build_jumppad()
        self.build_cycle = (self.build_cycle + 1) % 3
        return True


class FFSpy:
    def __init__(self):
        self.cloak = 100.0
        self.cloaked = False
        self.next_special_use = 0.0

    def think(self, now: float):
        if self.cloaked:
            if self.cloak > 0:
                self.cloak -= 4
            if self.cloak <= 0:
                self.cloak = 0
                self.cloaked = False
                self.next_special_use = now + 20
        elif now >= self.next_special_use and self.cloak < 100:
            self.cloak = 100.0

    def use_special(self, now: float) -> bool:
        if self.cloaked:
            self.cloaked = False
            self.cloak = 0
            self.next_special_use = now + 20
            return True
        if now < self.next_special_use or self.cloak <= 0:
            return False
        self.cloaked = True
        return True
