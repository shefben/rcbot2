#include "FFBot_SDKDefines.h"
#include "game/shared/shareddefs.h" // For actual CLASS_SCOUT, CLASS_SOLDIER, etc. from FF SDK
#include "game/shared/ff_weapon_base.h" // For FFWeaponID enum and AMMO_ defines (e.g. AMMO_CELLS_NAME)
#include "game/shared/ammodef.h"      // For CAmmoDef global instance (g_pAmmoDef)

#include <string>
#include <algorithm> // For std::transform
#include <map>       // For potential future map-based lookups if needed

// --- Implementations for mapping functions ---

FF_BotPlayerClassID GetBotClassIDFromString_FF(const std::string& className) {
    std::string lowerClassName = className;
    std::transform(lowerClassName.begin(), lowerClassName.end(), lowerClassName.begin(),
                   [](unsigned char c){ return std::tolower(c); });

    if (lowerClassName == "scout") return FF_BOT_CLASS_SCOUT;
    if (lowerClassName == "sniper") return FF_BOT_CLASS_SNIPER;
    if (lowerClassName == "soldier") return FF_BOT_CLASS_SOLDIER;
    if (lowerClassName == "demoman") return FF_BOT_CLASS_DEMOMAN;
    if (lowerClassName == "medic") return FF_BOT_CLASS_MEDIC;
    if (lowerClassName == "hwguy" || lowerClassName == "heavy" || lowerClassName == "hvyweap") return FF_BOT_CLASS_HVYWEAP; // FF uses HWGuy
    if (lowerClassName == "pyro") return FF_BOT_CLASS_PYRO;
    if (lowerClassName == "spy") return FF_BOT_CLASS_SPY;
    if (lowerClassName == "engineer") return FF_BOT_CLASS_ENGINEER;
    if (lowerClassName == "civilian") return FF_BOT_CLASS_CIVILIAN;

    return FF_BOT_CLASS_UNKNOWN;
}

// Example: If Fortress Forever's CSDKPlayerClassInfo (or similar) has an m_iSlot that matches CLASS_ defines
// This function might not be needed if direct string comparison or CLASS_ enum is used.
// FF_BotPlayerClassID GetBotClassIDFromSlot_FF(int ff_sdk_slot_id) {
//     // This assumes ff_sdk_slot_id directly corresponds to values in FF_BotPlayerClassID
//     // and that these values match the SDK's CLASS_ defines.
//     if (ff_sdk_slot_id >= FF_BOT_CLASS_SCOUT && ff_sdk_slot_id < FF_BOT_CLASS_MAX) {
//         return static_cast<FF_BotPlayerClassID>(ff_sdk_slot_id);
//     }
//     if (ff_sdk_slot_id == CLASS_NONE) return FF_BOT_CLASS_UNKNOWN; // Or CLASS_CIVILIAN if it's 0
//     return FF_BOT_CLASS_UNKNOWN;
// }

int GetSDKClassIDFromBotEnum_FF(FF_BotPlayerClassID botClassId) {
    // These CLASS_XXX constants must be the actual integer values from FF's shareddefs.h
    switch(botClassId) {
        case FF_BOT_CLASS_SCOUT:    return CLASS_SCOUT;
        case FF_BOT_CLASS_SNIPER:   return CLASS_SNIPER;
        case FF_BOT_CLASS_SOLDIER:  return CLASS_SOLDIER;
        case FF_BOT_CLASS_DEMOMAN:  return CLASS_DEMOMAN;
        case FF_BOT_CLASS_MEDIC:    return CLASS_MEDIC;
        case FF_BOT_CLASS_HVYWEAP:  return CLASS_HWGUY; // FF specific name for Heavy
        case FF_BOT_CLASS_PYRO:     return CLASS_PYRO;
        case FF_BOT_CLASS_SPY:      return CLASS_SPY;
        case FF_BOT_CLASS_ENGINEER: return CLASS_ENGINEER;
        case FF_BOT_CLASS_CIVILIAN: return CLASS_CIVILIAN; // Assuming CLASS_CIVILIAN is defined and used
        case FF_BOT_CLASS_UNKNOWN:
        default:
            return CLASS_NONE; // Or CLASS_CIVILIAN if that's the default/unknown in FF
    }
}

// --- Weapon and Ammo Mapping Function Implementations ---

int GetSDKWeaponIDFromString_FF(const std::string& weaponName) {
    // This function would map common weapon entity classnames (like "ff_weapon_rocketlauncher")
    // to the FFWeaponID enum values defined in ff_weapon_base.h or a similar SDK header.
    // This is highly specific to the SDK's weapon definitions.
    // Example (actual FFWeaponID enum values needed):
    if (weaponName == "ff_weapon_shovel") return FF_WEAPON_SHOVEL; // Assuming FF_WEAPON_SHOVEL is an enum value
    if (weaponName == "ff_weapon_shotgun_soldier") return FF_WEAPON_SHOTGUN_SOLDIER;
    if (weaponName == "ff_weapon_rocketlauncher") return FF_WEAPON_ROCKETLAUNCHER;
    if (weaponName == "ff_weapon_grenadelauncher") return FF_WEAPON_GRENADELAUNCHER;
    if (weaponName == "ff_weapon_pipelauncher") return FF_WEAPON_PIPES; // Or FF_WEAPON_PIPELAUNCHER
    if (weaponName == "ff_weapon_medikit") return FF_WEAPON_MEDIKIT;
    if (weaponName == "ff_weapon_syringegun") return FF_WEAPON_SYRINGEGUN;
    if (weaponName == "ff_weapon_minigun") return FF_WEAPON_MINIGUN;
    if (weaponName == "ff_weapon_flamethrower") return FF_WEAPON_FLAMETHROWER;
    if (weaponName == "ff_weapon_sniperrifle") return FF_WEAPON_SNIPERRIFLE;
    if (weaponName == "ff_weapon_revolver_spy") return FF_WEAPON_REVOLVER; // Assuming a general revolver ID if Spy's is unique
    if (weaponName == "ff_weapon_knife_spy") return FF_WEAPON_KNIFE;
    if (weaponName == "ff_weapon_pda_engineer") return FF_WEAPON_PDA_ENGY_BUILD;
    if (weaponName == "ff_weapon_wrench") return FF_WEAPON_WRENCH;
    if (weaponName == "ff_weapon_sapper") return FF_WEAPON_SAPPER;
    // ... add all relevant weapons ...
    return 0; // Or FF_WEAPON_NONE if it exists
}

const char* GetWeaponStringFromSDKID_FF(int weaponId) {
    // Reverse mapping for debugging or logging.
    // Needs actual FFWeaponID enum from ff_weapon_base.h
    switch (static_cast<FFWeaponID>(weaponId)) { // Cast to FFWeaponID to use in switch
        case FF_WEAPON_NONE: return "weapon_none";
        case FF_WEAPON_SHOVEL: return "ff_weapon_shovel";
        case FF_WEAPON_SHOTGUN_SOLDIER: return "ff_weapon_shotgun_soldier";
        case FF_WEAPON_ROCKETLAUNCHER: return "ff_weapon_rocketlauncher";
        // ... add all weapons ...
        default: return "unknown_weapon_id";
    }
}

int GetAmmoIndexForName_FF(const std::string& ammoName) {
    if (!g_pAmmoDef) { // g_pAmmoDef is the global instance of CAmmoDef from ammodef.h
        // Warning("GetAmmoIndexForName_FF: CAmmoDef (g_pAmmoDef) not available!\n");
        return -1; // Invalid index
    }
    return g_pAmmoDef->Index(ammoName.c_str());

    // Examples of ammoName strings (these are typically #defines in ff_weapon_base.h like AMMO_SHELLS_NAME "Shells")
    // "Shells" -> for shotguns
    // "Rockets" -> for rocket launcher
    // "Cells" -> for Engineer metal, Pyro flamethrower fuel (check specific game setup)
    // "Grenades" -> for Demoman grenades
    // "MedikitAmmo" -> conceptual for Medic healing resource if it uses ammo system
    // "SniperRifleAmmo" -> for Sniper
}

const char* GetPlayerClassNameFromBotID_FF(FF_BotPlayerClassID botClassId) {
    switch(botClassId) {
        case FF_BOT_CLASS_SCOUT:    return "scout";
        case FF_BOT_CLASS_SNIPER:   return "sniper";
        case FF_BOT_CLASS_SOLDIER:  return "soldier";
        case FF_BOT_CLASS_DEMOMAN:  return "demoman";
        case FF_BOT_CLASS_MEDIC:    return "medic";
        case FF_BOT_CLASS_HVYWEAP:  return "hwguy"; // FF specific name
        case FF_BOT_CLASS_PYRO:     return "pyro";
        case FF_BOT_CLASS_SPY:      return "spy";
        case FF_BOT_CLASS_ENGINEER: return "engineer";
        case FF_BOT_CLASS_CIVILIAN: return "civilian";
        default: return "unknown";
    }
}

std::string GetPlayerClassShortNameFromBotID_FF(FF_BotPlayerClassID botClassId) {
     switch(botClassId) {
        case FF_BOT_CLASS_SCOUT:    return "sc";
        case FF_BOT_CLASS_SNIPER:   return "sn";
        case FF_BOT_CLASS_SOLDIER:  return "sl";
        case FF_BOT_CLASS_DEMOMAN:  return "dm";
        case FF_BOT_CLASS_MEDIC:    return "md";
        case FF_BOT_CLASS_HVYWEAP:  return "hw";
        case FF_BOT_CLASS_PYRO:     return "py";
        case FF_BOT_CLASS_SPY:      return "sp";
        case FF_BOT_CLASS_ENGINEER: return "en";
        case FF_BOT_CLASS_CIVILIAN: return "cv";
        default: return "uk"; // Unknown
    }
}
