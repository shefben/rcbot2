#include "CFFPlayer.h" // Should be "CFFPlayerWrapper.h" after rename
// SDK Headers (ensure paths are correct relative to your FF source tree)
#include "public/engine/ivengineserver.h" // For g_pEngineServer (if used for ClientCommand)
#include "game/server/extdll_fns.h"     // For GetContainingEntity() or similar EDICT_TO_HIERARCHY
#include "game/server/util_shared.h"    // For UTIL_PlayerByIndex or similar if needed
#include "game/server/cbase.h"          // For CBaseEntity, CBasePlayer
#include "game/server/player.h"         // For CBasePlayer
#include "game/server/ff/ff_player.h"   // For ::CFFPlayer
#include "game/shared/ff_weapon_base.h" // For AMMO_ string defines and FFWeaponID enum
#include "game/shared/ammodef.h"      // For CAmmoDef and GetAmmoDef()
#include "game/shared/usercmd.h"
#include "game/shared/in_buttons.h"
#include "public/const.h"
#include "game/shared/shareddefs.h"   // For CLASS_ and FF_TEAM_ defines

// Bot Framework Headers
#include "FFBot_SDKDefines.h"
// #include "EngineInterfaces.h" // If g_pEngineServer is wrapped, but typically it's global from SDK itself

// Assume g_pEngineServer is accessible (e.g., via extern from CRCBotPlugin.cpp or an EngineInterfaces.h)
extern IVEngineServer* g_pEngineServer; // Make sure this is linked/available

CFFPlayerWrapper::CFFPlayerWrapper(edict_t* pBotEdict) :
    m_pEdict(pBotEdict),
    m_pSDKPlayer(nullptr) {
    UpdateSDKPlayerPtr();
}

void CFFPlayerWrapper::UpdateSDKPlayerPtr() {
    if (m_pEdict) {
        // Common Source SDK way to get CBaseEntity* from edict_t*
        // In FF, GetContainingEntity might be used, or CBaseEntity::Instance if available and edicts map directly.
        // CBaseEntity *pBaseEnt = GetContainingEntity(m_pEdict); // From extdll_fns.h usually
        // For newer SDKs or specific game branches:
        // IServerUnknown* pUnk = static_cast<IServerUnknown*>(m_pEdict->m_pUnk);
        // CBaseEntity* pBaseEnt = pUnk ? pUnk->GetBaseEntity() : nullptr;

        // Simpler conceptual placeholder if direct casting or a global map is assumed (not robust)
        // For Fortress Forever, CFFPlayer inherits CBasePlayer.
        // CBaseEntity::Instance(edict) is a common pattern in Source SDK.
        // If the edict IS the player entity directly (often true for players):
        if (m_pEdict && !m_pEdict->IsFree()) {
             CBaseEntity* pBaseEnt = CBaseEntity::Instance(m_pEdict); // SDK function
             if (pBaseEnt && pBaseEnt->IsPlayer()) {
                m_pSDKPlayer = static_cast<::CFFPlayer*>(pBaseEnt);
             } else {
                m_pSDKPlayer = nullptr;
             }
        } else {
            m_pSDKPlayer = nullptr;
        }
    } else {
        m_pSDKPlayer = nullptr;
    }
}

bool CFFPlayerWrapper::IsValid() const {
    // Re-check pointer validity, entity might have been removed
    // const_cast to allow UpdateSDKPlayerPtr to modify m_pSDKPlayer
    // const_cast<CFFPlayerWrapper*>(this)->UpdateSDKPlayerPtr();
    return m_pSDKPlayer != nullptr && m_pEdict != nullptr && !m_pEdict->IsFree();
}

bool CFFPlayerWrapper::IsAlive() const {
    return IsValid() && m_pSDKPlayer->IsAlive();
}

Vector CFFPlayerWrapper::GetOrigin() const {
    return IsValid() ? m_pSDKPlayer->GetAbsOrigin() : Vector(0,0,0); // Assuming SDK Vector
}

Vector CFFPlayerWrapper::GetVelocity() const {
    return IsValid() ? m_pSDKPlayer->GetAbsVelocity() : Vector(0,0,0); // Assuming SDK Vector
}

QAngle CFFPlayerWrapper::GetEyeAngles() const {
    return IsValid() ? m_pSDKPlayer->EyeAngles() : QAngle(0,0,0); // Assuming SDK QAngle
}

int CFFPlayerWrapper::GetHealth() const {
    return IsValid() ? m_pSDKPlayer->GetHealth() : 0;
}

int CFFPlayerWrapper::GetMaxHealth() const {
    // For FF, max health is often tied to class data
    // return IsValid() ? m_pSDKPlayer->GetMaxHealth() : 0; // CBasePlayer GetMaxHealth
    return IsValid() ? m_pSDKPlayer->m_PlayerClass.GetClassData()->m_iHealth : 0; // Using FF's class data struct
}

int CFFPlayerWrapper::GetArmor() const {
    return IsValid() ? m_pSDKPlayer->m_iArmor : 0; // Direct member access if public, or GetArmor()
}

int CFFPlayerWrapper::GetMaxArmor() const {
    // return IsValid() ? m_pSDKPlayer->m_iMaxArmor : 0; // Or from class data
    return IsValid() ? m_pSDKPlayer->m_PlayerClass.GetClassData()->m_iMaxArmour : 0;
}

int CFFPlayerWrapper::GetTeam() const {
    return IsValid() ? m_pSDKPlayer->GetTeamNumber() : TEAM_UNASSIGNED; // Use SDK TEAM_UNASSIGNED
}

int CFFPlayerWrapper::GetFlags() const {
    return IsValid() ? m_pSDKPlayer->GetFlags() : 0;
}

bool CFFPlayerWrapper::IsDucking() const {
    return (GetFlags() & FL_DUCKING) != 0;
}

bool CFFPlayerWrapper::IsOnGround() const {
    return (GetFlags() & FL_ONGROUND) != 0;
}

int CFFPlayerWrapper::GetAmmoByIndex(int ammoIndex) const {
    return IsValid() ? m_pSDKPlayer->GetAmmoCount(ammoIndex) : 0;
}

int CFFPlayerWrapper::GetAmmo(const std::string& ammoName) const {
    if (!IsValid()) return 0;
    // CAmmoDef::GetAmmoDef() is the standard Source SDK way.
    // FF might have its own ammo definition system or use this.
    int ammoIndex = 0; // Placeholder for CAmmoDef::GetAmmoDef()->Index(ammoName.c_str());
    // Need to include "shared/ammodef.h" and ensure CAmmoDef is initialized.
    // For now, using the mapping function.
    ammoIndex = GetAmmoIndexForName_FF(ammoName); // This needs to be implemented in FFBotUtils.cpp
    if (ammoIndex != -1) return GetAmmoByIndex(ammoIndex);
    return 0;
}

FF_BotPlayerClassID CFFPlayerWrapper::GetBotClassId() const {
    if (!IsValid()) return FF_BOT_CLASS_UNKNOWN;
    // m_pSDKPlayer->PlayerClass() might return C_PlayerClass* or similar.
    // m_pSDKPlayer->m_PlayerClass.GetClassNum() is typical for FF.
    return GetBotClassIDFromSlot_FF(m_pSDKPlayer->m_PlayerClass.GetClassNum());
}

std::string CFFPlayerWrapper::GetPlayerClassNameString() const {
    if (!IsValid()) return "unknown";
    return GetPlayerClassNameFromBotID_FF(GetBotClassId()); // Use our mapping
}

int CFFPlayerWrapper::GetSDKClassID() const {
    return IsValid() ? m_pSDKPlayer->m_PlayerClass.GetClassNum() : CLASS_NONE; // CLASS_NONE from shareddefs.h
}

float CFFPlayerWrapper::GetPlayerMaxSpeed() const {
    return IsValid() ? (float)m_pSDKPlayer->m_PlayerClass.GetClassData()->m_flSpeed : 0.0f; // m_flSpeed not m_iSpeed
}

int CFFPlayerWrapper::GetMetalCount() const {
    return GetAmmo(AMMO_CELLS_NAME); // AMMO_CELLS_NAME from ff_weapon_base.h (usually "Cells")
}

bool CFFPlayerWrapper::IsCloaked() const {
    return IsValid() && m_pSDKPlayer->IsCloaked(); // Direct SDK call
}

bool CFFPlayerWrapper::IsDisguised() const {
    return IsValid() && m_pSDKPlayer->IsDisguised(); // Direct SDK call
}

int CFFPlayerWrapper::GetDisguiseTeam() const {
    return IsValid() ? m_pSDKPlayer->GetDisguiseTeam() : TEAM_UNASSIGNED; // Direct SDK call
}

FF_BotPlayerClassID CFFPlayerWrapper::GetDisguiseClass() const {
    if (!IsValid() || !m_pSDKPlayer->IsDisguised()) return FF_BOT_CLASS_UNKNOWN;
    return GetBotClassIDFromSlot_FF(m_pSDKPlayer->GetDisguiseClass()); // Direct SDK call
}

float CFFPlayerWrapper::GetCloakEnergy() const {
    // return IsValid() ? m_pSDKPlayer->m_flCloakEnergy : 0.0f; // Direct member if public, or a GetCloakEnergy()
    return 0.0f; // Placeholder, depends on SDK CFFPlayer exact implementation
}

bool CFFPlayerWrapper::IsSapperActive_Conceptual() const {
    // if (!IsValid() || !m_pSDKPlayer->GetActiveWeapon()) return false;
    // return m_pSDKPlayer->GetActiveWeapon()->GetWeaponID() == FF_WEAPON_SAPPER; // FF_WEAPON_SAPPER from ff_weapon_base.h
    return false; // Placeholder
}

void CFFPlayerWrapper::SetViewAngles(CUserCmd* pCmd, const QAngle& angles) {
    if (pCmd) pCmd->viewangles = angles;
}

void CFFPlayerWrapper::SetMovement(CUserCmd* pCmd, float forwardMove, float sideMove, float upMove) {
    if (pCmd) {
        pCmd->forwardmove = forwardMove;
        pCmd->sidemove = sideMove;
        pCmd->upmove = upMove;
    }
}

void CFFPlayerWrapper::AddButton(CUserCmd* pCmd, int buttonFlag) {
    if (pCmd) pCmd->buttons |= buttonFlag;
}

void CFFPlayerWrapper::RemoveButton(CUserCmd* pCmd, int buttonFlag) {
    if (pCmd) pCmd->buttons &= ~buttonFlag;
}

void CFFPlayerWrapper::SetImpulse(CUserCmd* pCmd, unsigned char impulse) {
    if (pCmd) pCmd->impulse = impulse;
}

void CFFPlayerWrapper::SelectWeapon(CUserCmd* pCmd, const std::string& weaponClassName) {
    if (!pCmd || !IsValid()) return;
    // This usually involves mapping weaponClassName (e.g. "ff_weapon_rocketlauncher") to an SDK weapon ID or slot
    // int weaponSdkId = GetSDKWeaponIDFromString_FF(weaponClassName); // Implemented in FFBotUtils.cpp
    // if (weaponSdkId != 0 /* Assuming 0 is invalid/none or use specific FF_WEAPON_NONE */ ) {
    //    pCmd->weaponselect = weaponSdkId; // CUserCmd::weaponselect takes the weapon's entity index or a specific ID
    // } else {
        // Fallback: try "use weapon_classname" if direct select fails or for items not selected via weaponselect
        // This requires g_pEngineServer to be valid.
        // if (g_pEngineServer) {
        //     char cmd[128];
        //     snprintf(cmd, sizeof(cmd), "use %s", weaponClassName.c_str());
        //     g_pEngineServer->ClientCommand(m_pEdict, cmd); // This is an engine command, not filling CUserCmd directly
        // }
    // }
}

void CFFPlayerWrapper::PrimaryAttack(CUserCmd* pCmd) {
    AddButton(pCmd, IN_ATTACK);
}

void CFFPlayerWrapper::SecondaryAttack(CUserCmd* pCmd) {
    AddButton(pCmd, IN_ATTACK2);
}

void CFFPlayerWrapper::BuildBuilding_Command(BuildingType_FF buildingType) {
    if (!IsValid() || !g_pEngineServer) return; // Ensure g_pEngineServer is initialized and available
    const char* cmdStr = nullptr;
    switch(buildingType) {
        case BuildingType_FF::SENTRY_GUN: cmdStr = "buildsentry"; break;
        case BuildingType_FF::DISPENSER: cmdStr = "builddispenser"; break;
        case BuildingType_FF::TELEPORTER_ENTRANCE: cmdStr = "buildteleentrance"; break; // Check actual FF command
        case BuildingType_FF::TELEPORTER_EXIT: cmdStr = "buildteleexit"; break;     // Check actual FF command
        default: break;
    }
    if (cmdStr) {
        // Msg("CFFPlayerWrapper: Issuing build command: %s\n", cmdStr);
        g_pEngineServer->ClientCommand(m_pEdict, cmdStr);
    }
}

void CFFPlayerWrapper::DetonateBuildings_Command() {
    if (!IsValid() || !g_pEngineServer) return;
    // Msg("CFFPlayerWrapper: Issuing detonate buildings command.\n");
    g_pEngineServer->ClientCommand(m_pEdict, "det_engi_buildings"); // Use actual FF console command
}

void CFFPlayerWrapper::CloakToggle_Command() {
    if (!IsValid() || !g_pEngineServer) return;
    // Msg("CFFPlayerWrapper: Issuing cloak toggle command (special).\n");
    // In FF, Spy's cloak is often on the 'special' command, which maps to IN_ATTACK3 or similar logic in CFFPlayer::ItemPostFrame
    // Or it might be a direct "cloak" command. "special" is a common way.
    g_pEngineServer->ClientCommand(m_pEdict, "special");
}

void CFFPlayerWrapper::Disguise_Command(int ff_sdk_team_id, int ff_sdk_class_id) {
    if (!IsValid() || !g_pEngineServer) return;
    char cmd[64];
    snprintf(cmd, sizeof(cmd), "disguise %d %d", ff_sdk_team_id, ff_sdk_class_id); // Use SDK TEAM_ and CLASS_ defines
    // Msg("CFFPlayerWrapper: Issuing disguise command: %s\n", cmd);
    g_pEngineServer->ClientCommand(m_pEdict, cmd);
}
