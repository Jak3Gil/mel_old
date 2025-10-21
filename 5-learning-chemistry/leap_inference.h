#pragma once

#include "types.h"
#include "storage.h"
#include <vector>

namespace melvin {
namespace leap {

/**
 * 🧠 LEAP Inference Engine
 * 
 * Creates LEAP connections (educated guesses) from EXACT connections
 * 
 * Strategies:
 * 1. Transitivity: A→B→C ⇒ A--[LEAP]-->C
 * 2. Shared targets: A→C, B→C ⇒ A--[LEAP]-->B (similarity)
 * 3. Common neighbors: Similar connection patterns
 */
class LEAPInference {
public:
    struct Config {
        int max_transitive_hops;        // How deep to look for shortcuts
        int min_shared_neighbors;        // Min shared connections for similarity
        float leap_confidence;           // Confidence for LEAP edges
        bool verbose;                    // Print creation info
        
        Config() 
            : max_transitive_hops(3)
            , min_shared_neighbors(2)
            , leap_confidence(0.7f)
            , verbose(false)
        {}
    };
    
    explicit LEAPInference(const Config& config = Config());
    
    /**
     * Main entry point: Create LEAP connections from EXACT ones
     */
    int create_leap_connections(Storage* storage);
    
    /**
     * Strategy 1: Transitivity shortcuts
     * A→B→C creates A--[LEAP]-->C
     */
    int create_transitive_leaps(Storage* storage);
    
    /**
     * Strategy 2: Similarity from shared targets
     * A→C, B→C creates A--[LEAP]-->B
     */
    int create_similarity_leaps(Storage* storage);
    
    /**
     * Strategy 3: Common neighbor patterns
     * Nodes with similar connection patterns
     */
    int create_pattern_leaps(Storage* storage);
    
    /**
     * Get statistics
     */
    struct Stats {
        int transitive_leaps = 0;
        int similarity_leaps = 0;
        int pattern_leaps = 0;
        int total_leaps = 0;
    };
    
    Stats get_stats() const { return stats_; }
    
private:
    Config config_;
    Stats stats_;
    
    // Helper: Check if LEAP already exists
    bool leap_exists(Storage* storage, NodeID from, NodeID to);
};

} // namespace leap
} // namespace melvin

