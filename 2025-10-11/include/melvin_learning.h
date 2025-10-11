/*
 * MELVIN UNIFIED BRAIN - LEARNING LAYER
 * Reinforcement, evolution, and memory consolidation
 */

#pragma once
#include "melvin_storage.h"
#include "melvin_reasoning.h"

namespace melvin_learning {

using namespace melvin_storage;
using namespace melvin_reasoning;

// ==================== LEAP PROMOTION ====================

// Promote a LEAP edge to EXACT if it's been successful enough
inline bool promote_leap_to_exact(int edge_id) {
    if (edge_id < 0 || edge_id >= (int)edges.size()) return false;
    
    Edge& edge = edges[edge_id];
    
    if (!edge.is_leap()) return false;  // Already EXACT or not a leap
    
    // Check promotion criteria
    if (edge.w > melvin_reasoning::PROMOTE_THRESHOLD && 
        edge.successes >= melvin_reasoning::MIN_SUCCESSES) {
        
        std::cout << "   ✨ [PROMOTION] " << nodes[edge.a].data << " --LEAP--> " 
                  << nodes[edge.b].data << " → EXACT (w=" << edge.w 
                  << ", successes=" << edge.successes << ")\n";
        
        // Convert to EXACT
        edge.type = EdgeType::EXACT;
        edge.w_core = 0.5f;  // Reset to EXACT baseline
        edge.w_ctx = 0.5f;
        
        // Create bidirectional link (EXACT edges are bidirectional)
        Edge reverse_edge = edge;
        reverse_edge.a = edge.b;
        reverse_edge.b = edge.a;
        int reverse_id = (int)edges.size();
        edges.push_back(reverse_edge);
        adjacency[edge.b].push_back(reverse_id);
        
        return true;
    }
    
    return false;
}

// Delete weak LEAP edges that keep failing
inline bool prune_failed_leap(int edge_id) {
    if (edge_id < 0 || edge_id >= (int)edges.size()) return false;
    
    Edge& edge = edges[edge_id];
    
    if (!edge.is_leap()) return false;
    
    // Prune if weight is very low or too many failures
    if (edge.w < 0.1f || edge.failures > 5) {
        // Mark for deletion (we'll clean up later)
        edge.w = 0.0f;
        return true;
    }
    
    return false;
}

// ==================== REINFORCEMENT ====================

inline void reinforce_path(const std::vector<int>& path, float reward = 0.1f, bool success = true) {
    if (path.size() < 2) return;
    
    // Detect modalities used
    std::set<uint32_t> modalities_used;
    for (int node_id : path) {
        if (node_id >= 0 && node_id < (int)nodes.size()) {
            modalities_used.insert(nodes[node_id].sensory_type);
        }
    }
    
    // Bonus for multimodal fusion
    float multimodal_bonus = modalities_used.size() > 1 ? 1.2f : 1.0f;
    float adjusted_reward = reward * multimodal_bonus;
    
    // Find and reinforce edges in path
    for (size_t i = 0; i + 1 < path.size(); ++i) {
        int node_a = path[i];
        int node_b = path[i+1];
        
        // Find edge
        auto it = adjacency.find(node_a);
        if (it != adjacency.end()) {
            for (int edge_id : it->second) {
                if (edges[edge_id].b == node_b) {
                    reinforce_edge(edge_id, adjusted_reward);
                    
                    // Track success/failure for LEAP edges
                    if (edges[edge_id].is_leap()) {
                        if (success) {
                            edges[edge_id].successes++;
                            // Try to promote
                            promote_leap_to_exact(edge_id);
                        } else {
                            edges[edge_id].failures++;
                            // Try to prune
                            prune_failed_leap(edge_id);
                        }
                    }
                }
            }
        }
    }
    
    // Reinforce nodes in path
    for (int node_id : path) {
        if (node_id >= 0 && node_id < (int)nodes.size()) {
            nodes[node_id].weight = std::min(2.0f, nodes[node_id].weight + adjusted_reward * 0.05f);
        }
    }
}

// ==================== MEMORY DECAY ====================

inline void decay_pass(float decay_rate = 0.02f) {
    for (auto& edge : edges) {
        // Decay toward baseline
        edge.w_core = (1.0f - decay_rate) * edge.w_core + decay_rate * 0.1f;
        edge.w_ctx = (1.0f - decay_rate) * edge.w_ctx + decay_rate * 0.2f;
        edge.w = 0.3f * edge.w_core + 0.7f * edge.w_ctx;
        
        // Decay cross-modal bonus
        edge.cross_modal_bonus *= (1.0f - decay_rate);
    }
    
    for (auto& node : nodes) {
        node.weight *= (1.0f - decay_rate * 0.5f);
        if (node.weight < 0.1f) node.weight = 0.1f;  // Floor
    }
}

// ==================== EVOLUTION ====================

inline void evolve(int generation) {
    std::cout << "🧬 Evolution generation " << generation << "\n";
    
    // Find high-performing nodes (variance impact)
    std::vector<std::pair<float, int>> node_scores;
    for (size_t i = 0; i < nodes.size(); ++i) {
        float score = nodes[i].weight * nodes[i].reinforcements;
        node_scores.push_back({score, (int)i});
    }
    
    std::sort(node_scores.begin(), node_scores.end(), std::greater<>());
    
    // Boost top 10% nodes
    int boost_count = std::max(1, (int)(nodes.size() * 0.1));
    for (int i = 0; i < boost_count && i < (int)node_scores.size(); ++i) {
        int node_id = node_scores[i].second;
        nodes[node_id].weight = std::min(2.0f, nodes[node_id].weight * 1.1f);
    }
    
    // Prune weakest 5% edges
    std::vector<Edge> surviving_edges;
    for (const auto& edge : edges) {
        if (edge.w > 0.05f || edge.count > 2) {
            surviving_edges.push_back(edge);
        }
    }
    
    int pruned = edges.size() - surviving_edges.size();
    edges = surviving_edges;
    
    // Rebuild adjacency
    adjacency.clear();
    for (size_t i = 0; i < edges.size(); ++i) {
        adjacency[edges[i].a].push_back((int)i);
    }
    
    std::cout << "   Boosted " << boost_count << " nodes, pruned " << pruned << " edges\n";
}

// ==================== MEMORY CONSOLIDATION ====================

inline void consolidate_memory() {
    std::cout << "🧹 Consolidating memory...\n";
    
    // Strengthen frequently used edges
    for (auto& edge : edges) {
        if (edge.count > 10) {
            edge.w_core = std::min(1.0f, edge.w_core + 0.05f);
        }
    }
    
    // Merge duplicate nodes (same text)
    std::unordered_map<std::string, std::vector<int>> duplicates;
    for (size_t i = 0; i < nodes.size(); ++i) {
        duplicates[nodes[i].data].push_back(i);
    }
    
    int merged_count = 0;
    for (const auto& [data, ids] : duplicates) {
        if (ids.size() > 1) {
            // Keep the one with highest weight
            int keep_id = ids[0];
            float max_weight = nodes[ids[0]].weight;
            
            for (int id : ids) {
                if (nodes[id].weight > max_weight) {
                    max_weight = nodes[id].weight;
                    keep_id = id;
                }
            }
            
            // Redirect edges from duplicates to keeper
            for (auto& edge : edges) {
                for (int id : ids) {
                    if (id != keep_id) {
                        if (edge.a == id) edge.a = keep_id;
                        if (edge.b == id) edge.b = keep_id;
                        merged_count++;
                    }
                }
            }
        }
    }
    
    if (merged_count > 0) {
        std::cout << "   Merged " << merged_count << " duplicate node references\n";
    }
}

// ==================== SELF-EVALUATION ====================

inline float evaluate_coherence() {
    // Measure how well-connected the graph is
    if (nodes.empty() || edges.empty()) return 0.0f;
    
    float total_weight = 0.0f;
    for (const auto& edge : edges) {
        total_weight += edge.w;
    }
    
    float avg_weight = total_weight / edges.size();
    float connectivity = (float)edges.size() / (nodes.size() * nodes.size());
    
    return (avg_weight + connectivity) / 2.0f;
}

} // namespace melvin_learning

