#include "NavSystem.h"
#include "FFBaseAI.h" // For BotKnowledgeBase, assuming it includes FFStateStructs.h for Vector
                      // If BotKnowledgeBase is defined elsewhere, ensure Vector is known.
#include <iostream>   // For debug prints

// --- NavMeshGraph Implementation ---

NavMeshGraph::NavMeshGraph() {}

const NavAreaNode* NavMeshGraph::GetNode(unsigned int id) const {
    auto it = nodes.find(id);
    if (it != nodes.end()) {
        return &it->second;
    }
    return nullptr;
}

NavAreaNode* NavMeshGraph::GetNodeMutable(unsigned int id) {
    auto it = nodes.find(id);
    if (it != nodes.end()) {
        return &it->second;
    }
    return nullptr;
}

void NavMeshGraph::AddNode(const NavAreaNode& node) {
    nodes[node.id] = node; // This will overwrite if node.id already exists.
                           // Or use emplace: nodes.emplace(node.id, node);
}

void NavMeshGraph::AddConnection(unsigned int fromNodeId, unsigned int toNodeId,
                               NavConnectionType type, float costMultiplier, bool twoWay,
                               const Vector& fromPosOverride, const Vector& toPosOverride) {
    NavAreaNode* fromNode = GetNodeMutable(fromNodeId);
    NavAreaNode* toNode = GetNodeMutable(toNodeId);

    if (fromNode && toNode) {
        NavConnectionInternal conn(toNodeId, type, costMultiplier);
        if (fromPosOverride.x != 0 || fromPosOverride.y != 0 || fromPosOverride.z != 0) { // Check if override provided
            conn.fromPos = fromPosOverride;
        } else {
            conn.fromPos = fromNode->center; // Default to center
        }
        if (toPosOverride.x != 0 || toPosOverride.y != 0 || toPosOverride.z != 0) {
            conn.toPos = toPosOverride;
        } else {
            conn.toPos = toNode->center; // Default to center
        }
        fromNode->connections.push_back(conn);

        if (twoWay) {
            NavConnectionInternal reverseConn(fromNodeId, type, costMultiplier);
            reverseConn.fromPos = conn.toPos;   // Use the target's connection point as source for reverse
            reverseConn.toPos = conn.fromPos; // Use the source's connection point as target for reverse
            toNode->connections.push_back(reverseConn);
        }
    } else {
        // std::cerr << "NavMeshGraph Error: Attempted to add connection between non-existent nodes: "
        //           << fromNodeId << " -> " << toNodeId << std::endl;
    }
}

void NavMeshGraph::Clear() {
    nodes.clear();
}


// --- AStarPathfinder Implementation ---

AStarPathfinder::AStarPathfinder(const NavMeshGraph* pNavMeshGraph)
    : m_pNavMeshGraph(pNavMeshGraph) {
    if (!m_pNavMeshGraph) {
        // Consider throwing an exception or logging a critical error
        std::cerr << "AStarPathfinder Error: NavMeshGraph pointer is null!" << std::endl;
    }
}

float AStarPathfinder::CalculateHeuristic(const Vector& posA, const Vector& posB) {
    // Euclidean distance heuristic
    float dx = posA.x - posB.x;
    float dy = posA.y - posB.y;
    float dz = posA.z - posB.z;
    return std::sqrt(dx * dx + dy * dy + dz * dz);
}

float AStarPathfinder::CalculateMovementCost(const NavAreaNode* fromArea, const NavAreaNode* toArea,
                                           const NavConnectionInternal& connection,
                                           const BotKnowledgeBase* pKnowledgeBase) {
    if (!fromArea || !toArea) {
        return std::numeric_limits<float>::max(); // Should not happen if graph is consistent
    }

    // Base cost: Euclidean distance between area centers (or connection points if available)
    float distance;
    bool useCustomPositions = (connection.fromPos.LengthSquared() > 0.01f && connection.toPos.LengthSquared() > 0.01f);
    if(useCustomPositions){
        distance = connection.fromPos.Length(); // Incorrect, should be dist between fromPos and toPos
                                                 // Or more accurately, distance from fromArea->center to conn.fromPos + dist(conn.fromPos, conn.toPos) + dist(conn.toPos, toArea->center)
                                                 // For simplicity: use distance between the connection points themselves if they are set,
                                                 // otherwise, use distance between centers.
        float dx = connection.fromPos.x - connection.toPos.x;
        float dy = connection.fromPos.y - connection.toPos.y;
        float dz = connection.fromPos.z - connection.toPos.z;
        distance = std::sqrt(dx*dx + dy*dy + dz*dz);

    } else {
        float dx = fromArea->center.x - toArea->center.x;
        float dy = fromArea->center.y - toArea->center.y;
        float dz = fromArea->center.z - toArea->center.z;
        distance = std::sqrt(dx * dx + dy * dy + dz * dz);
    }


    float cost = distance * connection.costMultiplier;

    // Add cost for attributes of the destination area
    if (toArea->attributes & NavAttribute::CROUCH) cost += 20.0f; // Arbitrary extra cost for crouching
    if (toArea->attributes & NavAttribute::JUMP) cost += 30.0f;   // Arbitrary extra cost for areas requiring a jump to traverse
    if (toArea->attributes & NavAttribute::DANGER) cost += 100.0f; // Arbitrary extra cost for dangerous areas

    // Conceptual: Add dynamic costs from BotKnowledgeBase (e.g., enemy threat)
    // if (pKnowledgeBase) {
    //     float dynamicThreatCost = pKnowledgeBase->GetAreaThreat(toArea->id); // GetAreaThreat is conceptual
    //     cost += dynamicThreatCost;
    //     if (pKnowledgeBase->IsAreaNearObjective(toArea->id, currentObjective)) { // Conceptual
    //         cost *= 0.8f; // Prefer paths towards objective
    //     }
    // }

    return cost;
}

std::vector<unsigned int> AStarPathfinder::ReconstructPath(unsigned int targetAreaId,
                                                         const std::unordered_map<unsigned int, AStarNode>& nodeDataMap) {
    std::vector<unsigned int> path;
    unsigned int currentId = targetAreaId;

    auto it = nodeDataMap.find(currentId);
    while (it != nodeDataMap.end() && it->second.parentAreaId != 0) { // Assuming 0 is not a valid area ID or marks start
        path.push_back(currentId);
        currentId = it->second.parentAreaId;
        it = nodeDataMap.find(currentId);
    }
    if (currentId != 0) { // Add the start node itself (if not using 0 as sentinel)
         path.push_back(currentId);
    }
    std::reverse(path.begin(), path.end());
    return path;
}


std::vector<unsigned int> AStarPathfinder::FindPath(unsigned int startAreaId, unsigned int targetAreaId,
                                                  const BotKnowledgeBase* pKnowledgeBase) {
    if (!m_pNavMeshGraph) {
        std::cerr << "AStarPathfinder::FindPath Error: NavMeshGraph is null!" << std::endl;
        return {};
    }

    const NavAreaNode* startNavNode = m_pNavMeshGraph->GetNode(startAreaId);
    const NavAreaNode* targetNavNode = m_pNavMeshGraph->GetNode(targetAreaId);

    if (!startNavNode || !targetNavNode) {
        // std::cerr << "AStarPathfinder::FindPath Error: Invalid start or target Area ID." << std::endl;
        return {}; // Return empty path if start or target node doesn't exist
    }

    // Priority queue for the open set. Stores AStarNode directly.
    // std::priority_queue uses operator< by default for max-heap.
    // To get min-heap behavior (lowest fCost first), AStarNode::operator> is defined.
    std::priority_queue<AStarNode, std::vector<AStarNode>, std::greater<AStarNode>> openSetQueue;

    // Map to store AStarNode data (gCost, hCost, parent, flags) for all visited nodes.
    // Keyed by NavAreaNode ID.
    std::unordered_map<unsigned int, AStarNode> nodeDataMap;

    // Initialize start node
    AStarNode& startAStarNode = nodeDataMap[startAreaId]; // Creates node if not exists
    startAStarNode.areaId = startAreaId;
    // startAStarNode.position = startNavNode->center; // Not needed if fetched from graph
    startAStarNode.gCost = 0.0f;
    startAStarNode.hCost = CalculateHeuristic(startNavNode->center, targetNavNode->center);
    startAStarNode.fCost = startAStarNode.gCost + startAStarNode.hCost;
    startAStarNode.parentAreaId = 0; // No parent for start node (or use special ID)
    startAStarNode.inOpenSet = true;

    openSetQueue.push(startAStarNode);

    while (!openSetQueue.empty()) {
        AStarNode currentAStarNode = openSetQueue.top(); // Get node with lowest F cost
        openSetQueue.pop();

        // Check if this node instance in the queue is outdated (if we re-added a better path to it)
        // This check is important if we push updated nodes without being able to efficiently remove/update old ones.
        // AStarNode& mapNode = nodeDataMap[currentAStarNode.areaId];
        // if (currentAStarNode.fCost > mapNode.fCost && mapNode.inClosedSet) { // A better path to this was already found and processed or it's an old version
        //    continue;
        // }
        // More robustly, when using std::priority_queue which doesn't have decrease-key, we might push duplicates.
        // The mapNode.inClosedSet check below handles already fully processed nodes.
        // If we find a node in openSetQueue whose map version has a *better* fCost, this queue entry is stale.
        // However, the map always holds the best known gCost.

        nodeDataMap[currentAStarNode.areaId].inOpenSet = false; // Moved out of open set effectively
        nodeDataMap[currentAStarNode.areaId].inClosedSet = true;   // Add to closed set

        if (currentAStarNode.areaId == targetAreaId) {
            return ReconstructPath(targetAreaId, nodeDataMap); // Path found
        }

        const NavAreaNode* currentNavNode = m_pNavMeshGraph->GetNode(currentAStarNode.areaId);
        if (!currentNavNode) continue; // Should not happen

        for (const NavConnectionInternal& connection : currentNavNode->connections) {
            unsigned int neighborAreaId = connection.targetAreaId;
            const NavAreaNode* neighborNavNode = m_pNavMeshGraph->GetNode(neighborAreaId);

            if (!neighborNavNode || neighborNavNode->isBlocked) { // Skip invalid or blocked neighbors
                continue;
            }

            // Ensure node data exists or is created for neighbor
            AStarNode& neighborAStarNodeData = nodeDataMap[neighborAreaId];
            if (neighborAStarNodeData.areaId == 0) { // If newly created by map access
                neighborAStarNodeData.areaId = neighborAreaId;
                // neighborAStarNodeData.position = neighborNavNode->center; // Store if needed, or fetch
                neighborAStarNodeData.gCost = std::numeric_limits<float>::max(); // Initialize gCost to infinity
            }


            if (neighborAStarNodeData.inClosedSet) { // Already processed this neighbor via a shorter or equal path
                continue;
            }

            float tentativeGCost = currentAStarNode.gCost +
                                   CalculateMovementCost(currentNavNode, neighborNavNode, connection, pKnowledgeBase);

            if (tentativeGCost < neighborAStarNodeData.gCost) {
                neighborAStarNodeData.parentAreaId = currentAStarNode.areaId;
                neighborAStarNodeData.gCost = tentativeGCost;
                neighborAStarNodeData.hCost = CalculateHeuristic(neighborNavNode->center, targetNavNode->center);
                neighborAStarNodeData.fCost = neighborAStarNodeData.gCost + neighborAStarNodeData.hCost;

                if (!neighborAStarNodeData.inOpenSet) {
                    openSetQueue.push(neighborAStarNodeData);
                    neighborAStarNodeData.inOpenSet = true;
                } else {
                    // If using std::priority_queue, we can't efficiently update existing elements.
                    // One common approach is to push the updated node anyway. The outdated version
                    // will eventually be popped but likely ignored due to higher fCost or because
                    // the node is already in closed set by the time the stale one is processed.
                    // For this to work best, when popping from queue, check if node in map has better f_cost.
                    // For now, just re-pushing.
                    openSetQueue.push(neighborAStarNodeData);
                    // To make this more correct with standard priority_queue, when popping:
                    // if (currentAStarNode.fCost > nodeDataMap[currentAStarNode.areaId].fCost) continue; // Stale entry
                }
            }
        }
    }

    // std::cout << "AStarPathfinder::FindPath: Path not found from " << startAreaId << " to " << targetAreaId << std::endl;
    return {}; // Path not found
}
