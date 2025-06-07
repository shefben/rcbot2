#ifndef NAV_SYSTEM_H
#define NAV_SYSTEM_H

#include <vector>
#include <string>
#include <unordered_map>
#include <queue>   // For std::priority_queue
#include <cmath>   // For std::sqrt, std::fabs
#include <limits>  // For std::numeric_limits
#include <functional> // For std::hash with enum class

#include "FFStateStructs.h" // For Vector (ensure this is created and includes Vector definition)

// Forward declaration for BotKnowledgeBase to be used in dynamic cost calculation
struct BotKnowledgeBase;


// --- NavAreaNode and Connection Definitions ---
enum class NavAttribute : unsigned int { // Using enum class for type safety
    NONE = 0,
    CROUCH = 1 << 0, // Bot must crouch to traverse this area
    JUMP = 1 << 1,   // Bot must jump to traverse this area (usually for gaps, not up ledges)
    STOP = 1 << 2,   // Bot should stop when entering this area (e.g. elevator wait point)
    DANGER = 1 << 3, // Area is inherently dangerous (e.g. sniper sightline, under Sentry fire)
    BLOCKED = 1 << 4 // Area is currently blocked (e.g. by a door, or dynamically by game event)
    // Add more as needed: WATER, LADDER, OBJECTIVE_AREA, SNIPING_SPOT etc.
};

inline NavAttribute operator|(NavAttribute a, NavAttribute b) {
    return static_cast<NavAttribute>(static_cast<unsigned int>(a) | static_cast<unsigned int>(b));
}
inline NavAttribute& operator|=(NavAttribute& a, NavAttribute b) {
    a = a | b;
    return a;
}
inline bool operator&(NavAttribute a, NavAttribute b) {
    return static_cast<bool>(static_cast<unsigned int>(a) & static_cast<unsigned int>(b));
}


enum class NavConnectionType {
    WALK,
    JUMP_ONEWAY,      // A jump the bot can make one way (e.g., dropping down)
    JUMP_TWOWAY,      // A jump the bot can make both ways (requires more skill or specific setup)
    CROUCH_WALK,      // Must be crouching
    ELEVATOR_WAIT,
    ELEVATOR_RIDE,
    TELEPORTER_ENTRANCE,
    TELEPORTER_EXIT,
    CUSTOM_NAV_ABILITY // e.g., requires rocket jump, grapple hook, etc.
};

struct NavConnectionInternal {
    unsigned int targetAreaId;
    NavConnectionType type;
    Vector fromPos; // Optional: specific connection point in source area (e.g., edge midpoint)
    Vector toPos;   // Optional: specific connection point in target area (e.g., edge midpoint)
    float costMultiplier; // Base multiplier for this connection type (e.g., JUMP might be 1.5x walk distance)
    // Potentially add flags like "requiresDoorOpen" or "oneWayOnly" if not implicit in type

    NavConnectionInternal(unsigned int id, NavConnectionType t = NavConnectionType::WALK, float cost = 1.0f)
        : targetAreaId(id), type(t), costMultiplier(cost) {}
};

struct NavAreaNode {
    unsigned int id;        // Unique ID for this nav area
    Vector center;          // Geometric center of the nav area
    NavAttribute attributes; // Bitmask of NavAttribute flags
    bool isBlocked;         // Dynamically set if area becomes unusable (e.g., door closes)
    std::vector<NavConnectionInternal> connections; // Outgoing connections to other NavAreaNode IDs

    // Optional: Store AABB for more precise spatial queries
    // Vector mins, maxs;

    NavAreaNode(unsigned int _id = 0) : id(_id), attributes(NavAttribute::NONE), isBlocked(false) {}
};


// --- NavMeshGraph Definition ---
class NavMeshGraph {
public:
    std::unordered_map<unsigned int, NavAreaNode> nodes;

    NavMeshGraph(); // Constructor

    // Conceptual: bool LoadFromEngineNavMesh(void* pEngineNavMeshInterface);
    // This would iterate through the engine's nav areas and populate this graph.

    const NavAreaNode* GetNode(unsigned int id) const;
    NavAreaNode* GetNodeMutable(unsigned int id); // For adding connections

    void AddNode(const NavAreaNode& node); // For manual graph building or testing
    // Adds a connection from fromNodeId to toNodeId.
    void AddConnection(unsigned int fromNodeId, unsigned int toNodeId,
                       NavConnectionType type, float costMultiplier = 1.0f,
                       bool twoWay = false,
                       const Vector& fromPosOverride = Vector(), // Optional specific points for connection
                       const Vector& toPosOverride = Vector());

    void Clear(); // Clears all nodes and connections
};


// --- AStarPathfinder Definitions ---

// Internal node structure for A* algorithm
struct AStarNode {
    unsigned int areaId;    // The NavAreaNode ID this A* node represents
    // Vector position;    // Center of the nav area (can be fetched from NavMeshGraph if needed to save memory)
    float gCost;            // Cost from the start node to this node
    float hCost;            // Heuristic cost from this node to the target node
    float fCost;            // gCost + hCost
    unsigned int parentAreaId; // ID of the parent NavAreaNode in the path

    bool inOpenSet;         // True if this node is currently in the open set queue
    bool inClosedSet;       // True if this node has been processed (moved to closed set)

    AStarNode(unsigned int id = 0 /* INVALID_NAV_AREA_ID */, float g = 0.f, float h = 0.f, unsigned int parent = 0) :
        areaId(id), gCost(g), hCost(h), fCost(g + h),
        parentAreaId(parent), inOpenSet(false), inClosedSet(false) {}

    // Comparison for priority queue (min-heap based on fCost)
    // Note: std::priority_queue is a max-heap by default, so we need > for it to act as min-heap.
    bool operator>(const AStarNode& other) const {
        if (fCost != other.fCost) {
            return fCost > other.fCost;
        }
        return hCost > other.hCost; // Tie-breaker: prefer nodes closer to target by heuristic
    }
    // Less than operator for sorting or other containers if needed
    bool operator<(const AStarNode& other) const {
        if (fCost != other.fCost) {
            return fCost < other.fCost;
        }
        return hCost < other.hCost;
    }
};


class AStarPathfinder {
public:
    AStarPathfinder(const NavMeshGraph* pNavMeshGraph);

    // Finds a path of NavAreaNode IDs from startAreaId to targetAreaId.
    // pKnowledgeBase can be used to access dynamic costs (e.g., enemy presence).
    std::vector<unsigned int> FindPath(unsigned int startAreaId, unsigned int targetAreaId,
                                       const BotKnowledgeBase* pKnowledgeBase = nullptr); // pKB is optional

private:
    const NavMeshGraph* m_pNavMeshGraph; // Non-owning pointer to the navmesh data.

    float CalculateHeuristic(const Vector& posA, const Vector& posB);

    // Ensure BotKnowledgeBase is forward declared or NavSystem.h includes BotKnowledgeBase.h (already done via FFBaseAI.h indirectly)
    float CalculateMovementCost(const NavAreaNode* fromArea, const NavAreaNode* toArea,
                                const NavConnectionInternal& connection,
                                const BotKnowledgeBase* pKnowledgeBase); // pKB for dynamic costs, already matches

    std::vector<unsigned int> ReconstructPath(unsigned int targetAreaId,
                                              const std::unordered_map<unsigned int, AStarNode>& nodeDataMap); // Renamed param for clarity
};

#endif // NAV_SYSTEM_H
