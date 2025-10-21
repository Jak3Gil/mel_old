#pragma once

#include "../core/types_v2.h"
#include "../core/neuromodulators.h"
#include "../evolution/genome.h"
#include <vector>
#include <optional>

namespace melvin::v2::attention {

// ============================================================================
// ATTENTION ARBITRATION - Final Focus Selection
// ============================================================================

/**
 * AttentionArbitration combines bottom-up and top-down signals to select
 * a single focus target.
 * 
 * Formula (genome-driven):
 *   F = α·Saliency + β·Relevance + γ·Curiosity + δ·Need + ε·Persistence
 *       + ACh_gain·(bottom-up)
 *       + NE_explore·(random_noise)
 *       - IOR_penalty
 *       + 5-HT·inertia_bonus
 * 
 * Selection:
 *   - Softmax over scores (temperature from genome)
 *   - Inertia: require F_new > F_current × threshold
 *   - Inhibition of Return (IOR)
 */
class AttentionArbitration {
public:
    struct Config {
        // Attention formula weights (from genome)
        float alpha_saliency;
        float beta_relevance;
        float gamma_curiosity;
        float delta_need;
        float epsilon_persistence;
        
        // Dynamics
        float inertia_threshold;     // Switching threshold (1.15 = 15% better)
        float softmax_temperature;   // Selection stochasticity
        float ior_decay;             // Inhibition of return decay rate
        float ior_duration;          // How long to inhibit (seconds)
        
        Config()
            : alpha_saliency(0.40f), beta_relevance(0.30f), gamma_curiosity(0.20f),
              delta_need(0.05f), epsilon_persistence(0.05f),
              inertia_threshold(1.15f), softmax_temperature(0.2f),
              ior_decay(0.9f), ior_duration(2.0f) {}
    };
    
    AttentionArbitration(const evolution::Genome& genome);
    explicit AttentionArbitration(const Config& config);
    
    /**
     * Select focus from candidates
     * 
     * @param candidates - All perceived objects
     * @param saliency - Bottom-up scores
     * @param relevance - Top-down scores
     * @param neuromod - Current neuromodulator state
     * @returns Selected object ID (or 0 if none)
     */
    NodeID select_focus(
        const std::vector<PerceivedObject>& candidates,
        const std::vector<float>& saliency,
        const std::vector<float>& relevance,
        const NeuromodState& neuromod
    );
    
    /**
     * Get current focus
     */
    NodeID get_current_focus() const { return current_focus_; }
    
    /**
     * Get focus score
     */
    float get_focus_score() const { return current_focus_score_; }
    
    /**
     * Get focus duration (how long focused)
     */
    float get_focus_duration() const;
    
    /**
     * Force switch to new target (override inertia)
     */
    void force_switch(NodeID new_focus);
    
    /**
     * Reset (clear history)
     */
    void reset();
    
    void set_config(const Config& config);
    Config get_config() const;
    
    // ========================================================================
    // STATISTICS
    // ========================================================================
    
    struct Stats {
        size_t total_selections;
        size_t switches;              // Number of times focus changed
        size_t inertia_holds;         // Times inertia prevented switch
        size_t ior_suppressed;        // Items suppressed by IOR
        float avg_focus_duration;
        float avg_switch_interval;
    };
    
    Stats get_stats() const;
    void reset_stats();
    
private:
    Config config_;
    
    // Current focus state
    NodeID current_focus_;
    float current_focus_score_;
    Timestamp focus_start_time_;
    
    // Focus history (for IOR and persistence)
    struct FocusHistory {
        NodeID object_id;
        Timestamp focused_at;
        float duration;
    };
    std::vector<FocusHistory> history_;
    static constexpr size_t MAX_HISTORY = 30;
    
    Stats stats_;
    
    // Helpers
    float compute_score(
        const PerceivedObject& obj,
        float saliency,
        float relevance,
        const NeuromodState& neuromod
    );
    
    float get_ior_penalty(NodeID object_id, Timestamp current_time) const;
    float get_persistence_bonus(NodeID object_id) const;
    float softmax_sample(const std::vector<float>& scores);
    void update_history(NodeID selected, float duration);
};

// ============================================================================
// HELPER FUNCTIONS
// ============================================================================

/**
 * Extract attention config from genome
 */
AttentionArbitration::Config extract_attention_config(const evolution::Genome& genome);

} // namespace melvin::v2::attention

