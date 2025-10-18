#pragma once

#include "types.h"
#include "storage.h"
#include <string>
#include <vector>
#include <memory>

namespace melvin {

/**
 * 🧠 ReasoningEngine - LEAP + Multi-hop Reasoning
 * 
 * Implements emergent dimensional reasoning with:
 * - Context field dynamics
 * - Multi-hop path finding
 * - Beam search
 * - Emergent dimensional influences
 */
class ReasoningEngine {
public:
    struct Config {
        // Energy-based dynamic reasoning
        float energy_decay = 0.9f;          // Per-hop decay
        float energy_epsilon = 1e-5f;       // Min signal per edge
        float energy_floor = 1e-3f;         // Stop threshold
        float energy_cap = 10.0f;           // Runaway safety limit
        int time_budget_ms = 100;           // Time limit
        int max_hops_safety = 50;           // Hard hop limit
        
        // Beam search
        int beam_width = 8;
        float length_penalty = 0.9f;
        
        // LEAP parameters
        float leap_threshold = 0.7f;
        float abstraction_threshold = 0.78f;
        
        // Context field dynamics
        float diffusion_rate = 0.1f;
        float decay_rate = 0.95f;
        int diffusion_steps = 10;
        
        // Relation priors (learned weights)
        std::unordered_map<std::string, float> relation_priors;
        
        Config();
    };
    
    explicit ReasoningEngine(const Config& config = Config());
    ~ReasoningEngine();
    
    // ========================================================================
    // MAIN INFERENCE
    // ========================================================================
    
    Answer infer(const Query& query, Storage* storage);
    
    // Set paths for auto-save (enables persistence after each reasoning)
    void set_auto_save_paths(const std::string& nodes_path, const std::string& edges_path);
    
    // ========================================================================
    // PATH SEARCH
    // ========================================================================
    
    std::vector<ReasoningPath> find_paths(
        const NodeID& start,
        const NodeID& end,
        Storage* storage,
        int max_hops = 5
    );
    
    // ========================================================================
    // CONFIGURATION
    // ========================================================================
    
    void set_config(const Config& config);
    const Config& get_config() const;
    
    // ========================================================================
    // STATISTICS
    // ========================================================================
    
    struct Stats {
        uint64_t queries_processed = 0;
        uint64_t paths_explored = 0;
        uint64_t nodes_activated = 0;
        float avg_path_length = 0.0f;
        float avg_confidence = 0.0f;
        
        // Energy-based statistics
        float avg_hops_actual = 0.0f;       // Average actual hops taken
        float avg_energy_final = 0.0f;      // Average final energy
        uint64_t early_terminations = 0;    // Stopped before safety limit
        uint64_t safety_terminations = 0;   // Hit safety limit
    };
    
    Stats get_stats() const;
    void reset_stats();
    
private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace melvin

