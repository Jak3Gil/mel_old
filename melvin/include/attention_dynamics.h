/**
 * @file attention_dynamics.h
 * @brief Prevents attention from getting "stuck" on objects
 * 
 * Problem: Feedback loop causes staring
 *   Focus → Reinforces context → Increases β → More focus → Loop!
 * 
 * Solution: Multiple anti-sticking mechanisms
 *   1. Boredom: Reduce score for over-focused objects
 *   2. Context saturation: Boost curiosity when context stable
 *   3. Dynamic IOR: Stronger suppression for repeated targets
 *   4. Forced exploration: Switch after max duration
 */

#pragma once

#include "melvin_types.h"
#include <unordered_map>
#include <deque>
#include <cstdint>

namespace melvin {

/**
 * AttentionDynamics - Prevents getting stuck, encourages exploration
 * 
 * Biological parallels:
 * - Habituation: Repeated stimulus becomes less interesting
 * - Novelty seeking: Dopamine boost for exploration
 * - Saccadic inhibition: Can't fixate forever
 * - Boredom: Motivates attention shifts
 */
class AttentionDynamics {
public:
    AttentionDynamics();
    
    // ========================================================================
    // ANTI-STICKING MECHANISMS
    // ========================================================================
    
    /**
     * Record focus on object
     * Tracks how long we've been looking at it
     */
    void record_focus(uint64_t node_id);
    
    /**
     * Get boredom penalty for node
     * Returns negative score to reduce attention
     * 
     * @param node_id Node being considered
     * @return Penalty [0, -0.5], higher = more bored
     */
    float get_boredom_penalty(uint64_t node_id) const;
    
    /**
     * Get exploration bonus
     * Returns positive score for nodes NOT recently focused
     * 
     * @param node_id Node being considered
     * @return Bonus [0, 0.3], encourages exploration
     */
    float get_exploration_bonus(uint64_t node_id) const;
    
    /**
     * Check if should force context refresh
     * True if attention has been too stable
     */
    bool should_force_exploration() const;
    
    /**
     * Detect context saturation
     * True if same concepts active for too long
     */
    bool is_context_saturated(const std::vector<uint64_t>& active_concepts);
    
    /**
     * Get curiosity boost when context is stable
     * Encourages breaking out of mental loops
     * 
     * @return Curiosity multiplier [1.0, 2.0]
     */
    float get_saturation_curiosity_boost() const;
    
    /**
     * Update timers (call each frame)
     */
    void update(float dt);
    
    /**
     * Reset (when context cleared or major shift)
     */
    void reset();
    
    // ========================================================================
    // CONFIGURATION
    // ========================================================================
    
    struct Config {
        int max_focus_duration;           // Frames before boredom kicks in (default: 30)
        float boredom_rate;               // How fast boredom accumulates (default: 0.02)
        float exploration_bonus_rate;     // Bonus for unexplored (default: 0.01)
        int saturation_threshold;         // Frames before saturation (default: 20)
        float saturation_curiosity_mult;  // Curiosity boost when saturated (default: 1.5)
        
        Config()
            : max_focus_duration(30)
            , boredom_rate(0.02f)
            , exploration_bonus_rate(0.01f)
            , saturation_threshold(20)
            , saturation_curiosity_mult(1.5f)
        {}
    };
    
    void set_config(const Config& config);
    
private:
    Config config_;
    
    // Focus history tracking
    struct FocusRecord {
        uint64_t node_id;
        int consecutive_frames;
        float boredom_level;
    };
    
    std::deque<uint64_t> focus_history_;  // Last N focused nodes
    std::unordered_map<uint64_t, FocusRecord> focus_records_;
    
    uint64_t current_focus_;
    int frames_on_current_;
    
    // Context saturation tracking
    std::deque<size_t> context_size_history_;  // Track context stability
    int stable_context_frames_;
};

} // namespace melvin

