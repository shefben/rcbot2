#ifndef BOT_KNOWLEDGE_BASE_H
#define BOT_KNOWLEDGE_BASE_H

#include <vector>
#include <string>
#include <map>
#include <memory> // For std::unique_ptr if NavMeshGraph is heap allocated by KB

#include "FFStateStructs.h"   // For Vector, ControlPointInfo, PayloadPathInfo, ClassConfigInfo
#include "NavSystem.h"        // For NavMeshGraph, NavAreaNode, etc.
#include "TrackedEntityInfo.h"// For TrackedEntityInfo

// Forward declare lua_State for Lua interaction methods
struct lua_State;

// Conceptual Game Mode enum (if not defined globally elsewhere)
enum class GameModeType_KB { UNKNOWN, CONTROL_POINT, CAPTURE_THE_FLAG, PAYLOAD_ATTACK, PAYLOAD_DEFEND };


class BotKnowledgeBase {
public:
    BotKnowledgeBase();
    // ~BotKnowledgeBase(); // If managing raw pointers directly (e.g. m_NavGraph if it was raw)

    // --- Initialization & Update ---

    // Called once at plugin load or equivalent to load static data like class configs
    // Assumes Lua state is passed for FFLuaBridge interaction
    bool LoadGlobalClassConfigs(lua_State* L, const std::vector<std::string>& classConfigTableNames);

    // Called at map load
    // For NavMesh, conceptual: bool LoadNavMesh(const char* mapName, void* pEngineNavMeshInterface);
    bool LoadNavMesh(const char* mapName); // Simpler version for now, assumes internal/dummy loading
    bool LoadMapObjectiveData(lua_State* L, const char* mapName, const std::vector<std::string>& cpTableNames); // Pass list of CP table names

    // Called periodically or by events to update dynamic state
    void UpdateControlPointState(int cpId, int newOwnerTeam, float newCaptureProgress, bool newIsLocked);
    // void UpdatePayloadState(/* ...payload params... */);
    // void UpdateFlagState(/* ...flag params... */);

    // Updates the lists of tracked entities based on current perception input
    void UpdateTrackedEntities(const std::vector<TrackedEntityInfo>& currentTrackedEntities, int botTeamId);

    void ClearDynamicMapData(); // Called on LevelShutdown (clears CPs, Payload, Tracked Entities)
    void ClearAllData();        // Called on Unload (clears everything including configs, navmesh)


    // --- Accessors ---
    const NavMeshGraph* GetNavGraph() const { return &m_NavGraph; }
    NavMeshGraph* GetNavGraphMutable() { return &m_NavGraph; } // For loading

    const std::vector<ControlPointInfo>& GetControlPoints() const { return m_ControlPoints; }
    const ControlPointInfo* GetControlPoint(int cpId) const; // By internal ID
    const ControlPointInfo* GetControlPointByLuaName(const std::string& luaName) const;


    // const std::vector<PayloadPathInfo>& GetPayloadPaths() const { return m_PayloadPaths; }
    // ... Getters for Flags ...

    const std::vector<ClassConfigInfo>& GetClassConfigs() const { return m_ClassConfigs; }
    const ClassConfigInfo* GetClassConfigByName(const std::string& className) const;
    const ClassConfigInfo* GetClassConfigById(int classId) const;

    const std::vector<TrackedEntityInfo>& GetTrackedEnemies() const { return m_TrackedEnemies; }
    const std::vector<TrackedEntityInfo>& GetTrackedAllies() const { return m_TrackedAllies; }
    const TrackedEntityInfo* GetTrackedEntity(edict_t* pEdict) const; // Find by edict
    const TrackedEntityInfo* GetTrackedEntityById(int entityId) const; // Find by entityId


    // --- Utility / Game State Queries (Conceptual) ---
    GameModeType_KB GetCurrentGameMode() const { return m_CurrentGameMode; }
    void SetCurrentGameMode(GameModeType_KB mode) { m_CurrentGameMode = mode; }
    // float GetAreaThreat(unsigned int navAreaId) const; // Example for A* dynamic costs
    // bool IsAreaNearObjective(unsigned int navAreaId, const HighLevelTask* currentTask) const;


private:
    NavMeshGraph m_NavGraph; // KB owns the nav graph instance

    std::vector<ControlPointInfo> m_ControlPoints;
    std::vector<PayloadPathInfo> m_PayloadPaths;
    // std::vector<FlagInfo> m_Flags;

    std::vector<ClassConfigInfo> m_ClassConfigs; // Loaded once typically

    // Dynamic entities (cleared/updated often)
    std::vector<TrackedEntityInfo> m_TrackedEnemies;
    std::vector<TrackedEntityInfo> m_TrackedAllies;
    // For faster edict-based lookup if many entities are tracked:
    std::map<edict_t*, TrackedEntityInfo*> m_TrackedEntityMap; // Pointers to entities within the above vectors

    GameModeType_KB m_CurrentGameMode;
};

#endif // BOT_KNOWLEDGE_BASE_H
