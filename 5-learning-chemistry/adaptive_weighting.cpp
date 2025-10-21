/*
 * Adaptive Local Weighting Implementation
 * 
 * Scales weights using LOCAL normalization (not global)
 * Prevents weight dilution in large graphs
 */

#include "adaptive_weighting.h"
#include <iostream>
#include <cmath>

namespace melvin {
namespace adaptive {

AdaptiveWeighting::AdaptiveWeighting(const Config& config) 
    : config_(config) {}

void AdaptiveWeighting::update_weights(
    std::vector<Node>& nodes,
    std::vector<Edge>& edges,
    Storage* storage
) {
    cycle_count_++;
    
    // Only update every N cycles
    if (cycle_count_ % config_.update_interval != 0) {
        return;
    }
    
    if (config_.verbose) {
        std::cout << "[ADAPTIVE] Updating weights (cycle " << cycle_count_ << ")\n";
    }
    
    // Step 1: Update node weights using LOCAL normalization
    for (auto& node : nodes) {
        // Sum activations of neighbors only (not all nodes!)
        float neighbor_sum = 0.0f;
        auto out_edges = storage->get_edges_from(node.id);
        
        for (const auto& edge : out_edges) {
            // Find target node
            for (const auto& target : nodes) {
                if (target.id == edge.to_id) {
                    neighbor_sum += target.activations;
                    break;
                }
            }
        }
        
        // Local normalization formula
        // w = (activations^α) / (neighbor_sum^β)
        float numerator = std::pow(node.activations + 1.0f, config_.alpha);
        float denominator = std::pow(neighbor_sum + 1.0f, config_.beta);
        
        node.weight = numerator / denominator;
    }
    
    // Step 2: Update edge weights using LOCAL normalization
    for (auto& edge : edges) {
        // Sum coactivations of all edges from same source
        float edge_sum = 0.0f;
        
        for (const auto& other_edge : edges) {
            if (other_edge.from_id == edge.from_id) {
                edge_sum += other_edge.coactivations;
            }
        }
        
        // Local normalization formula
        // w = (coactivations^α) / (edge_sum^β)
        float numerator = std::pow(edge.coactivations + 1.0f, config_.alpha);
        float denominator = std::pow(edge_sum + 1.0f, config_.beta);
        
        edge.adaptive_weight = numerator / denominator;
    }
    
    // Compute statistics
    float total_node_weight = 0.0f;
    float total_edge_weight = 0.0f;
    uint64_t total_acts = 0;
    uint64_t total_coacts = 0;
    
    for (const auto& node : nodes) {
        total_node_weight += node.weight;
        total_acts += node.activations;
    }
    
    for (const auto& edge : edges) {
        total_edge_weight += edge.adaptive_weight;
        total_coacts += edge.coactivations;
    }
    
    stats_.avg_node_weight = nodes.empty() ? 0.0f : total_node_weight / nodes.size();
    stats_.avg_edge_weight = edges.empty() ? 0.0f : total_edge_weight / edges.size();
    stats_.total_activations = total_acts;
    stats_.total_coactivations = total_coacts;
    stats_.updates_performed++;
    
    if (config_.verbose) {
        std::cout << "[ADAPTIVE] Avg node weight: " << stats_.avg_node_weight << "\n";
        std::cout << "[ADAPTIVE] Avg edge weight: " << stats_.avg_edge_weight << "\n";
    }
}

void AdaptiveWeighting::apply_decay(
    std::vector<Node>& nodes,
    std::vector<Edge>& edges
) {
    // Prevent old data from dominating
    for (auto& node : nodes) {
        node.activations = static_cast<uint64_t>(node.activations * config_.decay);
    }
    
    for (auto& edge : edges) {
        edge.coactivations = static_cast<uint64_t>(edge.coactivations * config_.decay);
    }
    
    if (config_.verbose) {
        std::cout << "[ADAPTIVE] Applied decay (" << config_.decay << ")\n";
    }
}

void AdaptiveWeighting::record_activation(
    std::vector<Node>& nodes,
    const std::vector<NodeID>& active_node_ids
) {
    // Increment activation counters
    for (NodeID id : active_node_ids) {
        for (auto& node : nodes) {
            if (node.id == id) {
                node.activations++;
                break;
            }
        }
    }
}

void AdaptiveWeighting::record_coactivations(
    std::vector<Node>& nodes,
    std::vector<Edge>& edges,
    Storage* storage
) {
    // Find which nodes are currently active
    std::unordered_map<NodeID, bool> is_active;
    
    for (const auto& node : nodes) {
        if (node.activation > 0.0f) {
            is_active[node.id] = true;
        }
    }
    
    // Increment coactivation counters for edges between active nodes
    for (auto& edge : edges) {
        if (is_active[edge.from_id] && is_active[edge.to_id]) {
            edge.coactivations++;
        }
    }
    
    (void)storage; // Unused for now
}

} // namespace adaptive
} // namespace melvin

