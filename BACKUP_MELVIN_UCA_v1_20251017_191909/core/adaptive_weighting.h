#pragma once

#include "types.h"
#include "storage.h"
#include <cmath>
#include <unordered_map>

namespace melvin {
namespace adaptive {

/**
 * 📊 Adaptive Local Weighting System
 * 
 * Automatically scales node and edge weights based on:
 * - Local activation patterns (not global!)
 * - Co-activation frequency
 * - Neighbor normalization
 * 
 * Formula:
 *   w_node = (activations^α) / (neighbor_sum^β)
 *   w_edge = (coactivations^α) / (edge_sum^β)
 * 
 * Where:
 *   α (alpha) = emphasis on local count (default: 1.0)
 *   β (beta) = scaling to stabilize growth (default: 0.3)
 */
class AdaptiveWeighting {
public:
    struct Config {
        float alpha;              // Emphasis on local count
        float beta;               // Scaling factor
        float decay;              // Prevent old data domination
        int update_interval;      // Update every N cycles
        bool verbose;             // Log updates
        
        Config()
            : alpha(1.0f)
            , beta(0.3f)
            , decay(0.999f)
            , update_interval(10)
            , verbose(false)
        {}
    };
    
    explicit AdaptiveWeighting(const Config& config = Config());
    
    /**
     * Update adaptive weights for all nodes and edges
     * Uses local normalization (not global)
     */
    void update_weights(
        std::vector<Node>& nodes,
        std::vector<Edge>& edges,
        Storage* storage
    );
    
    /**
     * Apply decay to prevent old connections from dominating
     */
    void apply_decay(
        std::vector<Node>& nodes,
        std::vector<Edge>& edges
    );
    
    /**
     * Increment activation counters during reasoning
     */
    void record_activation(
        std::vector<Node>& nodes,
        const std::vector<NodeID>& active_node_ids
    );
    
    /**
     * Increment coactivation counters for edges
     */
    void record_coactivations(
        std::vector<Node>& nodes,
        std::vector<Edge>& edges,
        Storage* storage
    );
    
    /**
     * Get statistics
     */
    struct Stats {
        float avg_node_weight = 0.0f;
        float avg_edge_weight = 0.0f;
        uint64_t total_activations = 0;
        uint64_t total_coactivations = 0;
        int updates_performed = 0;
    };
    
    Stats get_stats() const { return stats_; }
    void reset_stats() { stats_ = Stats(); }
    
private:
    Config config_;
    Stats stats_;
    int cycle_count_ = 0;
};

} // namespace adaptive
} // namespace melvin

