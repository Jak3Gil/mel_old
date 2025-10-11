#pragma once

#include "storage.h"
#include "melvin_leap_nodes.h"
#include <vector>
#include <string>
#include <unordered_map>

// Forward declarations (global scope, consistent with other modules)
struct Node;
struct Edge;

namespace melvin {

class LeapController;

namespace learning {

// Called after each reasoning path to apply learning updates
GrowthStats apply_learning_updates(
    std::vector<uint64_t>& path,
    float entropy_before,
    float entropy_after,
    float similarity,
    std::unordered_map<uint64_t, Node>& G_nodes,
    std::vector<Edge>& G_edges,
    LeapController* leap_controller
);

// Update edge weights along a successful reasoning path
void bump_edge_weights(
    const std::vector<uint64_t>& path,
    std::vector<Edge>& edges,
    float success_signal  // 0.0-1.0, higher = more successful
);

// Create a new node if needed (when token doesn't match existing nodes well)
uint64_t create_node_if_needed(
    const std::string& token,
    const std::vector<float>& activation_vector,
    std::unordered_map<uint64_t, Node>& nodes,
    float similarity_threshold = 0.55
);

// Try to promote a temporary leap node to permanent
bool try_promote_leap(
    LeapController* leap,
    uint64_t& promoted_node_id
);

// Compute cosine similarity between two vectors
float cosine_similarity(const std::vector<float>& a, const std::vector<float>& b);

} // namespace learning
} // namespace melvin

