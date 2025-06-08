#ifndef BOT_KNOWLEDGE_BASE_H
#define BOT_KNOWLEDGE_BASE_H

#include <vector>
#include <string>
#include <map>
#include <memory>

#include "FFStateStructs.h"
#include "NavSystem.h"
#include "TrackedEntityInfo.h"
#include "FFEngineerAI.h" // For BuildingType_FF (Ideally, BuildingType_FF is in a more general header if KB needs it)

struct lua_State;
// enum class GameModeType_KB { UNKNOWN, CONTROL_POINT, CAPTURE_THE_FLAG, PAYLOAD_ATTACK, PAYLOAD_DEFEND };

// --- Projectile Tracking (Conceptual) ---
struct ReflectableProjectileInfo {
    edict_t* pEntity = nullptr;    // Pointer to the projectile's game entity
    int projectileId = -1;         // Unique ID if available
    Vector position;
    Vector velocity;
    float creationTime = 0.0f;     // Game time when projectile was created or first seen
    float estimatedImpactTime = -1.0f; // Estimated time to reach a critical target or area
    int projectileType = 0;        // Conceptual: e.g., ROCKET, GRENADE, STICKYBOMB
    bool isHostile = false;        // True if it's an enemy projectile
    // Potentially: edict_t* pPredictedTargetEdict; // If we can predict what it's aiming for
};


// New enum and struct for building information
enum class BuildingStatus_FF {
    UNKNOWN,
    CONSTRUCTION_GHOST, // Blueprint placed, not yet being built
    BUILDING,           // Being hit by wrench to construct
    ACTIVE,             // Fully built and operational
    SAPPED,             // Has a sapper on it
    UPGRADING,          // Being hit by wrench to upgrade level
    DESTROYED           // Not present or destroyed
};

struct BuildingInfo {
    edict_t* pEdict;            // Conceptual pointer to the building's game entity
    int uniqueId;               // If engine provides one (e.g. netprop), otherwise use edict index or hash
    BuildingType_FF type;
    int level;                  // Typically 1-3 for Sentry/Dispenser/Teleporter
    int health;
    int maxHealth;              // Can depend on type and level
    int builderPlayerId;        // UserID of the CFFPlayer who built it (0 if not player built/unknown)
    int teamId;                 // Team affiliation
    Vector position;
    Vector angles; // QAngle or Vector for orientation

    bool isSapped;
    bool isBuildingInProgress;   // True if it's a blueprint being built (health increasing from 0)
    bool isUpgrading;            // True if currently receiving wrench hits for next level
    // float nextUpgradeMetalRequirement; // Metal needed for next level progress
    // float repairMetalCost;          // Current metal cost to repair 1 unit of health (can vary)

    BuildingInfo() :
        pEdict(nullptr), uniqueId(-1), type(BuildingType_FF::UNKNOWN), level(0),
        health(0), maxHealth(0), builderPlayerId(-1), teamId(0),
        isSapped(false), isBuildingInProgress(false), isUpgrading(false) {}
};


class BotKnowledgeBase {
public:
    BotKnowledgeBase();
    // ~BotKnowledgeBase();

    // --- Initialization & Update ---
    bool LoadGlobalClassConfigs(lua_State* L, const std::vector<std::string>& classConfigTableNames);
    bool LoadNavMesh(const char* mapName);
    bool LoadMapObjectiveData(lua_State* L, const char* mapName, const std::vector<std::string>& cpTableNames);

    void UpdateControlPointState(int cpId, int newOwnerTeam, float newCaptureProgress, bool newIsLocked);
    void UpdateTrackedEntities(const std::vector<TrackedEntityInfo>& currentTrackedEntities, int botTeamId); // May be deprecated by UpdateTrackedPlayers
    void UpdateTrackedPlayers(const std::vector<TrackedEntityInfo>& perceivedPlayers);     // Renamed
    void UpdateTrackedBuildings(const std::vector<BuildingInfo>& perceivedBuildings); // Renamed
    void UpdateTrackedProjectiles(const std::vector<ReflectableProjectileInfo>& currentProjectiles);

    // Building specific updates
    void UpdateOrAddBuilding(const BuildingInfo& newBuildingInfo); // May be wrapped or replaced by UpdateTrackedBuildings
    void RemoveBuilding(edict_t* pEdict); // Or by uniqueId
    void UpdateBuildingHealth(edict_t* pEdict, int newHealth, bool isStillBuilding); // Also updates isBuildingInProgress
    void UpdateBuildingSapped(edict_t* pEdict, bool isSapped);
    void UpdateBuildingLevel(edict_t* pEdict, int newLevel);


    void ClearDynamicMapData();
    void ClearAllData();

    // --- Accessors ---
    const NavMeshGraph* GetNavGraph() const { return &m_NavGraph; }
    NavMeshGraph* GetNavGraphMutable() { return &m_NavGraph; }
    const std::vector<ControlPointInfo>& GetControlPoints() const { return m_ControlPoints; }
    const ControlPointInfo* GetControlPoint(int cpId) const;
    const ControlPointInfo* GetControlPointByLuaName(const std::string& luaName) const;
    const std::vector<ClassConfigInfo>& GetClassConfigs() const { return m_ClassConfigs; }
    const ClassConfigInfo* GetClassConfigByName(const std::string& className) const;
    const ClassConfigInfo* GetClassConfigById(int classId) const;
    const std::vector<TrackedEntityInfo>& GetTrackedEnemies() const { return m_TrackedEnemies; }
    const std::vector<TrackedEntityInfo>& GetTrackedAllies() const { return m_TrackedAllies; }
    const TrackedEntityInfo* GetTrackedEntity(edict_t* pEdict) const;
    const TrackedEntityInfo* GetTrackedEntityById(int entityId) const;
    const std::vector<ReflectableProjectileInfo>& GetTrackedProjectiles() const; // New

    // Building specific accessors
    const BuildingInfo* GetBuildingInfo(edict_t* pEdict) const;
    const BuildingInfo* GetBuildingInfoByUniqueId(int uniqueId) const;
    std::vector<const BuildingInfo*> GetFriendlyBuildings(int botTeamId) const;
    std::vector<const BuildingInfo*> GetFriendlyBuildingsOfType(BuildingType_FF type, int botTeamId) const;
    std::vector<const BuildingInfo*> GetEnemyBuildingsInArea(const Vector& areaCenter, float radius, int botTeamId) const;
    std::vector<const BuildingInfo*> GetOwnBuildings(int botPlayerId) const; // Buildings built by a specific bot


    GameModeType_KB GetCurrentGameMode() const { return m_CurrentGameMode; }
    void SetCurrentGameMode(GameModeType_KB mode) { m_CurrentGameMode = mode; }
    // ... (Conceptual GetAreaThreat, IsAreaNearObjective) ...

private:
    void CreatePlaceholderNavMesh(); // Moved from .cpp to be callable by constructor if needed

    NavMeshGraph m_NavGraph;
    std::vector<ControlPointInfo> m_ControlPoints;
    std::vector<PayloadPathInfo> m_PayloadPaths;
    std::vector<ClassConfigInfo> m_ClassConfigs;
    std::vector<TrackedEntityInfo> m_TrackedEnemies;
    std::vector<TrackedEntityInfo> m_TrackedAllies;
    std::vector<ReflectableProjectileInfo> m_TrackedProjectiles; // New
    // std::map<edict_t*, TrackedEntityInfo*> m_TrackedEntityMap; // Pointers to entities within the above vectors

    std::vector<BuildingInfo> m_TrackedBuildings; // Stores all known buildings on map
    // For faster lookup of buildings by edict (if edicts are reliable keys)
    // std::map<edict_t*, size_t> m_BuildingEdictToIndexMap;

    GameModeType_KB m_CurrentGameMode;
};

#endif // BOT_KNOWLEDGE_BASE_H
