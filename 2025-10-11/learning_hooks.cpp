#include "learning_hooks.h"
#include <algorithm>
#include <cmath>
#include <iostream>

// Full Node and Edge definitions (global scope, synchronized with other modules)
struct Node {
    uint64_t id = 0;
    std::string text;
    uint32_t type = 0;
    int freq = 0;
    bool pinned = false;
    std::vector<float> emb;
    std::vector<float> embedding;
    float attention_weight = 0.0f;
    uint64_t last_accessed = 0;
    float semantic_strength = 1.0f;
    float activation = 0.0f;
};

struct Edge {
    uint64_t u, v;
    uint64_t locB;
    float weight;
    float w_core;
    float w_ctx;
    uint32_t count;
    
    void update_frequency(uint64_t total_nodes) { (void)total_nodes; }
};

namespace melvin {
namespace learning {

// Compute cosine similarity between two vectors
float cosine_similarity(const std::vector<float>& a, const std::vector<float>& b) {
    if (a.empty() || b.empty() || a.size() != b.size()) {
        return 0.0f;
    }
    
    float dot = 0.0f;
    float mag_a = 0.0f;
    float mag_b = 0.0f;
    
    for (size_t i = 0; i < a.size(); i++) {
        dot += a[i] * b[i];
        mag_a += a[i] * a[i];
        mag_b += b[i] * b[i];
    }
    
    mag_a = std::sqrt(mag_a);
    mag_b = std::sqrt(mag_b);
    
    if (mag_a < 1e-10f || mag_b < 1e-10f) {
        return 0.0f;
    }
    
    return dot / (mag_a * mag_b);
}

// Update edge weights along a successful reasoning path
void bump_edge_weights(
    const std::vector<uint64_t>& path,
    std::vector<Edge>& edges,
    float success_signal
) {
    if (path.size() < 2) {
        return; // Need at least 2 nodes to form an edge
    }
    
    // For each consecutive pair in path
    for (size_t i = 0; i + 1 < path.size(); i++) {
        uint64_t from = path[i];
        uint64_t to = path[i+1];
        
        // Find edge (or create)
        auto it = std::find_if(edges.begin(), edges.end(),
            [from, to](const Edge& e) { return e.u == from && e.v == to; });
        
        if (it != edges.end()) {
            // Update existing edge
            it->count++;
            it->weight += success_signal * 0.1f;  // Small increment
            it->weight = std::min(it->weight, 1.0f);
            it->w_core = it->weight; // Update core weight too
        } else {
            // Create new edge
            Edge new_edge;
            new_edge.u = from;
            new_edge.v = to;
            new_edge.locB = to;
            new_edge.weight = 0.5f + success_signal * 0.2f;
            new_edge.w_core = new_edge.weight;
            new_edge.w_ctx = 0.0f;
            new_edge.count = 1;
            edges.push_back(new_edge);
        }
    }
}

// Create a new node if needed (when token doesn't match existing nodes well)
uint64_t create_node_if_needed(
    const std::string& token,
    const std::vector<float>& activation_vector,
    std::unordered_map<uint64_t, Node>& nodes,
    float similarity_threshold
) {
    if (token.empty()) {
        return 0; // Invalid token
    }
    
    // Check if any existing node matches
    float best_sim = 0.0f;
    uint64_t best_id = 0;
    
    for (const auto& [id, node] : nodes) {
        // Exact text match
        if (node.text == token) {
            nodes[id].freq++; // Increment frequency
            return id;
        }
        
        // Check embedding similarity if both have embeddings
        if (!node.embedding.empty() && !activation_vector.empty()) {
            float sim = cosine_similarity(node.embedding, activation_vector);
            if (sim > best_sim) {
                best_sim = sim;
                best_id = id;
            }
        }
    }
    
    // Create new node if similarity too low
    if (best_sim < similarity_threshold) {
        // Find max ID in the map
        uint64_t max_id = 0;
        for (const auto& [id, node] : nodes) {
            if (id > max_id) max_id = id;
        }
        uint64_t new_id = max_id + 1;
        
        Node new_node;
        new_node.id = new_id;
        new_node.text = token;
        new_node.embedding = activation_vector;
        new_node.emb = activation_vector; // Duplicate for compatibility
        new_node.freq = 1;
        new_node.type = 0;
        new_node.pinned = false;
        new_node.attention_weight = 0.0f;
        new_node.last_accessed = 0;
        new_node.semantic_strength = 1.0f;
        new_node.activation = 0.0f;
        
        nodes[new_id] = new_node;
        return new_id;
    }
    
    // Return best match if above threshold
    if (best_id > 0) {
        nodes[best_id].freq++; // Increment frequency
    }
    return best_id;
}

// Try to promote a temporary leap node to permanent
bool try_promote_leap(
    LeapController* leap,
    uint64_t& promoted_node_id
) {
    // For now, this is a stub - full implementation would interact with LeapController
    // to find temporary leaps with high success rates and promote them
    
    // Check if leap controller has any successful patterns to promote
    if (leap == nullptr) {
        return false;
    }
    
    // Future implementation:
    // - Check leap success rates
    // - Find leaps with success >= threshold over K occurrences
    // - Create permanent generalized node
    // - Connect with LEAP_GENERALIZATION edges
    
    promoted_node_id = 0;
    return false;
}

// Main learning update function called after each reasoning step
GrowthStats apply_learning_updates(
    std::vector<uint64_t>& path,
    float entropy_before,
    float entropy_after,
    float similarity,
    std::unordered_map<uint64_t, Node>& G_nodes,
    std::vector<Edge>& G_edges,
    LeapController* leap_controller
) {
    GrowthStats stats;
    
    if (path.empty()) {
        return stats; // No path to learn from
    }
    
    uint32_t initial_node_count = G_nodes.size();
    uint32_t initial_edge_count = G_edges.size();
    
    // Compute success signal from entropy reduction and similarity
    float entropy_reduction = std::max(0.0f, entropy_before - entropy_after);
    float success_signal = (entropy_reduction * 0.5f + similarity * 0.5f);
    success_signal = std::clamp(success_signal, 0.0f, 1.0f);
    
    // Update edge weights along the path
    bump_edge_weights(path, G_edges, success_signal);
    
    // Count new edges
    uint32_t final_edge_count = G_edges.size();
    if (final_edge_count > initial_edge_count) {
        stats.edges_added = final_edge_count - initial_edge_count;
    } else {
        stats.edges_updated = path.size() > 1 ? path.size() - 1 : 0;
    }
    
    // Try to promote leaps if we have a successful reasoning path
    if (success_signal > 0.7f && leap_controller != nullptr) {
        uint64_t promoted_id = 0;
        if (try_promote_leap(leap_controller, promoted_id)) {
            stats.leaps_promoted++;
        }
    }
    
    // Count new nodes (for future node creation logic)
    uint32_t final_node_count = G_nodes.size();
    if (final_node_count > initial_node_count) {
        stats.nodes_added = final_node_count - initial_node_count;
        
        // Collect labels of new nodes (up to 3 for display)
        for (const auto& [id, node] : G_nodes) {
            if (id > initial_node_count && stats.new_node_labels.size() < 3) {
                stats.new_node_labels.push_back(node.text);
            }
        }
    }
    
    return stats;
}

} // namespace learning
} // namespace melvin

