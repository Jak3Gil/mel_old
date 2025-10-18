#pragma once

#include <string>
#include <vector>
#include <cmath>
#include <algorithm>

namespace melvin {

/**
 * =============================
 * MELVIN ATTENTION SYSTEM
 * =============================
 * 
 * Cognitive gatekeeper deciding which sensory or internal event deserves focus.
 * Calculates, ranks, and selects focus targets based on relevance, salience, and need.
 */

struct FocusCandidate {
    std::string label;       // e.g. "object_5", "sound_loud", "thought_fire"
    float A;                 // Attention salience (motion, brightness, loudness)
    float R;                 // Relevance to current task
    float N;                 // Need satisfaction potential (power, safety, goal)
    float T;                 // Temporal persistence / duration
    float C;                 // Curiosity / unpredictability
    double timestamp;        // Time seen or generated
    
    // Spatial information (for visual candidates)
    int x;                   // Center X position
    int y;                   // Center Y position
    int area;                // Size (for visual objects)
    
    // Source information
    std::string source;      // "vision", "audio", "internal", etc.
    
    // Computed focus score
    float focus_score;
    
    FocusCandidate() 
        : A(0), R(0), N(0), T(0), C(0), timestamp(0), 
          x(0), y(0), area(0), focus_score(0) {}
};

struct AttentionWeights {
    float wA;  // Sensory salience
    float wR;  // Goal relevance
    float wN;  // Internal need
    float wT;  // Temporal duration
    float wC;  // Curiosity
    
    // Default weights
    AttentionWeights() 
        : wA(0.40f), wR(0.30f), wN(0.20f), wT(0.05f), wC(0.05f) {}
    
    void normalize() {
        float sum = wA + wR + wN + wT + wC;
        if (sum > 0) {
            wA /= sum;
            wR /= sum;
            wN /= sum;
            wT /= sum;
            wC /= sum;
        }
    }
};

enum class MelvinState {
    EXPLORING,       // High curiosity, low goal focus
    TASK_FOCUSED,    // High relevance, low curiosity
    LOW_ENERGY,      // High need, low sensory
    OVERSTIMULATED,  // Low salience, filtering mode
    BALANCED         // Default balanced weights
};

class AttentionManager {
public:
    AttentionManager();
    ~AttentionManager();
    
    // ========================================================================
    // FOCUS CANDIDATE MANAGEMENT
    // ========================================================================
    
    /**
     * Add a new focus candidate to the queue
     */
    void add_candidate(const FocusCandidate& candidate);
    
    /**
     * Clear all candidates (start fresh frame)
     */
    void clear_candidates();
    
    // ========================================================================
    // FOCUS SELECTION
    // ========================================================================
    
    /**
     * Compute focus scores for all candidates and select best
     * Returns: selected FocusCandidate (or empty if none)
     */
    FocusCandidate select_focus_target(double current_time);
    
    /**
     * Get current focus target
     */
    const FocusCandidate& get_current_focus() const { return current_focus_; }
    
    /**
     * Check if there is an active focus
     */
    bool has_focus() const { return !current_focus_.label.empty(); }
    
    // ========================================================================
    // STATE MANAGEMENT
    // ========================================================================
    
    /**
     * Set Melvin's current cognitive state (affects weight balance)
     */
    void set_state(MelvinState state);
    
    /**
     * Get current state
     */
    MelvinState get_state() const { return state_; }
    
    /**
     * Manually adjust weights (for fine-tuning)
     */
    void set_weights(const AttentionWeights& weights);
    
    /**
     * Get current weights
     */
    const AttentionWeights& get_weights() const { return weights_; }
    
    // ========================================================================
    // METRICS
    // ========================================================================
    
    /**
     * Get focus history (last N focus targets)
     */
    std::vector<FocusCandidate> get_focus_history(int count = 10) const;
    
    /**
     * Print current attention state
     */
    void print_status() const;
    
private:
    // ========================================================================
    // INTERNAL METHODS
    // ========================================================================
    
    /**
     * Compute focus score for a candidate
     */
    float compute_focus_score(const FocusCandidate& candidate) const;
    
    /**
     * Apply temporal decay to old candidates
     */
    void apply_temporal_decay(FocusCandidate& candidate, double current_time) const;
    
    /**
     * Normalize scores between 0-1
     */
    void normalize_scores(std::vector<FocusCandidate>& candidates) const;
    
    /**
     * Update weights based on current state
     */
    void update_weights_for_state();
    
    // ========================================================================
    // STATE
    // ========================================================================
    
    std::vector<FocusCandidate> candidates_;   // Current frame candidates
    FocusCandidate current_focus_;             // Currently focused target
    std::vector<FocusCandidate> focus_history_; // Past focus targets
    
    AttentionWeights weights_;                 // Current attention weights
    MelvinState state_;                        // Current cognitive state
    
    // Temporal decay parameter
    float decay_rate_;  // k in exp(-k * Δt)
    
    // Maximum history size
    static constexpr int MAX_HISTORY = 100;
};

} // namespace melvin

