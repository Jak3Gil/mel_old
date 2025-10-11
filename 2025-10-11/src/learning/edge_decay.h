#pragma once

#include <vector>
#include <string>

// Forward declaration
struct Edge;

namespace melvin {
namespace learning {

// Decay options
struct DecayOptions {
    float lambda = 0.015f;  // Decay rate (per day)
    float w_min = 0.1f;     // Minimum weight (don't decay to zero)
    bool prune_low = false; // Remove edges below w_min
    bool verbose = true;
};

// Decay result
struct DecayResult {
    uint32_t edges_decayed = 0;
    uint32_t edges_pruned = 0;
    float avg_weight_before = 0.0f;
    float avg_weight_after = 0.0f;
};

// Edge decay manager (Ebbinghaus forgetting curve)
class EdgeDecay {
public:
    // Apply decay to all edges based on time since last use
    static DecayResult apply_decay(
        std::vector<::Edge>& edges,
        const std::string& current_date_iso,
        const DecayOptions& opts = DecayOptions()
    );
    
    // Rehearse edges (anti-decay on success)
    static void rehearse_edges(
        std::vector<::Edge>& edges,
        const std::vector<uint64_t>& path,
        bool success,
        float alpha = 0.05f,  // Boost on success
        float beta = 0.8f     // Penalty on failure
    );
    
private:
    // Compute decay factor using Ebbinghaus formula
    static float compute_decay_factor(
        float days_since_use,
        float lambda
    );
    
    // Get current date (ISO 8601)
    static std::string current_date_iso();
    
    // Compute days between two ISO dates
    static float days_between(
        const std::string& date1_iso,
        const std::string& date2_iso
    );
};

} // namespace learning
} // namespace melvin

