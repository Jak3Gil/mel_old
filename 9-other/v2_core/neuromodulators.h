#pragma once

#include "types_v2.h"
#include "../evolution/genome.h"
#include <memory>

namespace melvin::v2 {

// ============================================================================
// NEUROMODULATORS - The "Chemistry" of Cognition
// ============================================================================

/**
 * Neuromodulators implements four key neuromodulatory systems inspired by biology:
 * 
 * - Dopamine (DA):      Reward prediction error → gates plasticity (learning rate)
 * - Norepinephrine (NE): Unexpected uncertainty → exploration & attention reset
 * - Acetylcholine (ACh): Expected uncertainty → sharpens bottom-up attention
 * - Serotonin (5-HT):    Stability/patience → suppresses impulsive switching
 * 
 * All parameters come from the genome, making neuromodulation evolvable.
 */
class Neuromodulators {
public:
    struct Config {
        // Baseline levels (from genome)
        float da_baseline;
        float ne_baseline;
        float ach_baseline;
        float serotonin_baseline;
        
        // Gain parameters (sensitivity to events)
        float da_gain;
        float ne_gain;
        float ach_gain;
        float serotonin_gain;
        
        // Decay rates (homeostatic return to baseline)
        float da_decay;
        float ne_decay;
        float ach_decay;
        float serotonin_decay;
        
        // Bounds
        float min_level;
        float max_level;
        
        Config()
            : da_baseline(0.5f), ne_baseline(0.5f), 
              ach_baseline(0.5f), serotonin_baseline(0.5f),
              da_gain(0.5f), ne_gain(0.5f), 
              ach_gain(0.5f), serotonin_gain(0.5f),
              da_decay(0.95f), ne_decay(0.90f), 
              ach_decay(0.92f), serotonin_decay(0.98f),
              min_level(0.0f), max_level(1.0f) {}
    };
    
    Neuromodulators(const evolution::Genome& genome);
    explicit Neuromodulators(const Config& config);
    ~Neuromodulators();
    
    // ========================================================================
    // EVENT-DRIVEN UPDATES
    // ========================================================================
    
    /**
     * Update from prediction error
     * - Positive error (unexpected reward) → DA ↑
     * - Negative error (unexpected punishment) → DA ↓
     * - Large absolute error → NE ↑ (unexpected uncertainty)
     */
    void on_prediction_error(float error);
    
    /**
     * Update from unexpected event
     * - Novel stimuli → NE ↑ (reset, exploration)
     */
    void on_unexpected_event(float surprise_magnitude = 1.0f);
    
    /**
     * Update from expected uncertainty
     * - Ambiguous stimuli → ACh ↑ (sharpen attention)
     */
    void on_expected_uncertainty(float ambiguity);
    
    /**
     * Update from goal progress
     * - Progress → DA ↑, 5-HT ↑
     * - Lack of progress → 5-HT ↓ (less stable, try new strategies)
     */
    void on_goal_progress(float progress_delta);
    
    /**
     * Update from conflict/frustration
     * - High conflict → 5-HT ↓ (reduce patience)
     */
    void on_conflict(float conflict_magnitude);
    
    // ========================================================================
    // STATE ACCESS
    // ========================================================================
    
    /**
     * Get current neuromodulator state
     */
    NeuromodState get_state() const;
    
    /**
     * Get individual levels
     */
    float get_dopamine() const { return state_.dopamine; }
    float get_norepinephrine() const { return state_.norepinephrine; }
    float get_acetylcholine() const { return state_.acetylcholine; }
    float get_serotonin() const { return state_.serotonin; }
    
    // ========================================================================
    // COMPUTED EFFECTS (What neuromodulators DO)
    // ========================================================================
    
    /**
     * Get plasticity rate (DA-modulated learning rate)
     * - High DA → faster learning
     * - Low DA → slower learning / forgetting
     */
    float get_plasticity_rate() const;
    
    /**
     * Get exploration bias (NE-modulated)
     * - High NE → explore more, widen attention
     * - Low NE → exploit, narrow focus
     */
    float get_exploration_bias() const;
    
    /**
     * Get attention gain (ACh-modulated)
     * - High ACh → boost bottom-up saliency
     * - Low ACh → reduce sensory influence
     */
    float get_attention_gain() const;
    
    /**
     * Get stability bias (5-HT-modulated)
     * - High 5-HT → suppress impulsive switches
     * - Low 5-HT → more willing to change strategy
     */
    float get_stability_bias() const;
    
    /**
     * Get effective learning rate (combines plasticity with gains)
     * - Used to gate Hebbian updates, LEAP formation, etc.
     */
    float get_effective_learning_rate(float base_rate = 0.01f) const;
    
    // ========================================================================
    // TIME-BASED UPDATES
    // ========================================================================
    
    /**
     * Tick: homeostatic regulation (decay toward baseline)
     * - Called once per cognitive cycle
     * - dt = delta time in seconds
     */
    void tick(float dt);
    
    /**
     * Reset to baseline levels
     */
    void reset();
    
    // ========================================================================
    // CONFIGURATION
    // ========================================================================
    
    void set_config(const Config& config);
    Config get_config() const;
    
    // ========================================================================
    // STATISTICS
    // ========================================================================
    
    struct Stats {
        float total_prediction_errors;
        float total_unexpected_events;
        float total_conflicts;
        float avg_dopamine;
        float avg_norepinephrine;
        float avg_acetylcholine;
        float avg_serotonin;
        size_t tick_count;
    };
    
    Stats get_stats() const;
    void reset_stats();
    
private:
    Config config_;
    NeuromodState state_;
    Stats stats_;
    
    // Running averages for stats
    float sum_da_;
    float sum_ne_;
    float sum_ach_;
    float sum_5ht_;
    size_t sample_count_;
    
    // Clamp to valid range
    void clamp_all();
    
    // Apply homeostatic decay
    void apply_decay(float dt);
};

// ============================================================================
// HELPER FUNCTIONS
// ============================================================================

/**
 * Extract neuromodulator config from genome
 */
Neuromodulators::Config extract_neuromod_config(const evolution::Genome& genome);

/**
 * Pretty-print neuromodulator state
 */
std::string neuromod_state_to_string(const NeuromodState& state);

} // namespace melvin::v2

