#ifndef ENGINE_INTERFACES_H
#define ENGINE_INTERFACES_H

// Forward declarations for Source Engine interfaces
class IVEngineServer;
class IPlayerInfoManager;
class IServerGameClients;
class CGlobalVarsBase;    // For things like curtime, tickinterval
class IEngineTrace;
class ICvar; // For ConVars/ConCommands
// Add other interfaces as needed, e.g., IGameEventManager2

// Conceptual Global Pointers to Engine Interfaces
// These would be initialized in CRCBotPlugin::Load()
extern IVEngineServer*       g_pEngineServer;
extern IPlayerInfoManager*   g_pPlayerInfoManager;
extern IServerGameClients*   g_pServerGameClients;
extern CGlobalVarsBase*      g_pGlobals;
extern IEngineTrace*         g_pEngineTraceClient;
extern ICvar*                g_pCVar;

// Helper function to get CBaseEntity* from edict_t* (conceptual)
// In a real Source SDK, this might be: CBaseEntity::Instance(edict_t*)
// or server->PEntityOfEntIndex( engine->IndexOfEdict(edict_t*) )
class CBaseEntity; // Forward declare
inline CBaseEntity* GetBaseEntityFromEdict(edict_t* pEdict) {
    // if (g_pEngineServer && pEdict) {
    //     int entIndex = g_pEngineServer->IndexOfEdict(pEdict); // Conceptual
    //     if (entIndex >= 0) return g_pEngineServer->PEntityOfEntIndex(entIndex); // Conceptual
    // }
    return nullptr; // Placeholder
}

#endif // ENGINE_INTERFACES_H
