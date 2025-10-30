#ifndef THREE_CHANNEL_FEEDBACK_H
#define THREE_CHANNEL_FEEDBACK_H

#include <vector>
#include <deque>
#include <unordered_map>
#include <chrono>

namespace melvin {
namespace cognitive_field {

/**
 * Three-Channel Feedback System
 * 
 * Separates feedback into three distinct channels:
 * 1. Sensory Feedback - Physical consequences (camera, mic, motor states)
 * 2. Cognitive Feedback - Prediction errors update attention bias
 * 3. Evolutionary Feedback - Sustained success adjusts genome parameters
 * 
 * This replaces the shallow "echo text" feedback with true reflection
 * and multi-level adaptation.
 */

// ============================================================================
// Channel 1: Sensory Feedback
// ============================================================================

struct SensoryFeedback {
    std::chrono::high_resolution_clock::time_point timestamp;
    
    // Physical state observations
    std::vector<float> vision_state;      // Current camera input
    std::vector<float> audio_state;       // Current microphone input
    std::vector<float> motor_state;       // Current motor positions/velocities
    std::vector<float> proprio_state;     // Proprioceptive feedback
    
    // Consequences of previous actions
    int action_node_id;                   // What action was taken
    float action_outcome;                 // Success metric (0-1)
    std::vector<int> changed_nodes;       // What nodes changed as a result
    std::vector<float> change_magnitudes; // How much they changed
    
    SensoryFeedback() : action_node_id(-1), action_outcome(0.0f) {}
};

class SensoryFeedbackChannel {
public:
    /**
     * Inject sensory feedback from environment
     * Re-injects physical consequences as new activations
     */
    void inject_sensory_feedback(const SensoryFeedback& feedback);
    
    /**
     * Compare predicted state to actual state
     * Returns prediction error for each modality
     */
    struct PredictionError {
        float vision_error;
        float audio_error;
        float motor_error;
        float total_error;
    };
    
    PredictionError compute_prediction_error(
        const std::vector<float>& predicted_vision,
        const std::vector<float>& actual_vision,
        const std::vector<float>& predicted_audio,
        const std::vector<float>& actual_audio,
        const std::vector<float>& predicted_motor,
        const std::vector<float>& actual_motor) const;
    
    /**
     * Track action-outcome pairs for motor learning
     */
    void record_action_outcome(int action_node_id, float outcome);
    
    /**
     * Get recent feedback history
     */
    std::vector<SensoryFeedback> get_recent_feedback(size_t count) const;
    
private:
    std::deque<SensoryFeedback> feedback_history_;
    static constexpr size_t MAX_HISTORY = 1000;
    
    // Action-outcome statistics for each action
    std::unordered_map<int, std::pair<float, int>> action_success_rates_;
};

// ============================================================================
// Channel 2: Cognitive Feedback
// ============================================================================

struct CognitiveFeedback {
    std::chrono::high_resolution_clock::time_point timestamp;
    
    // Prediction-error signals
    int predicted_node;                   // What was predicted
    int actual_node;                      // What actually happened
    float prediction_confidence;          // How confident was the prediction
    float prediction_error;               // Magnitude of error
    
    // Context at prediction time
    std::vector<int> context_nodes;       // What nodes were active
    std::vector<float> context_activations;
    
    // Attention adjustments
    std::vector<int> boost_nodes;         // Nodes to increase attention on
    std::vector<int> suppress_nodes;      // Nodes to decrease attention on
    std::vector<float> attention_deltas;  // How much to adjust
    
    CognitiveFeedback() : predicted_node(-1), actual_node(-1),
                         prediction_confidence(0.0f), prediction_error(0.0f) {}
};

class CognitiveFeedbackChannel {
public:
    /**
     * Send prediction error back to update attention bias
     * This is the core of "learning from mistakes"
     */
    void send_prediction_error(const CognitiveFeedback& feedback);
    
    /**
     * Compute attention adjustments based on prediction errors
     * Boost nodes that would have helped, suppress nodes that misled
     */
    std::unordered_map<int, float> compute_attention_adjustments() const;
    
    /**
     * Get surprise signal (average prediction error)
     * High surprise -> increase exploration
     */
    float get_surprise_level() const;
    
    /**
     * Get coherence signal (prediction stability)
     * High coherence -> confident in current understanding
     */
    float get_coherence_level() const;
    
    /**
     * Reset attention adjustments (after consolidation)
     */
    void reset_attention_adjustments();
    
private:
    std::deque<CognitiveFeedback> cognitive_history_;
    static constexpr size_t MAX_HISTORY = 500;
    
    // Accumulated attention adjustments
    std::unordered_map<int, float> attention_bias_;
    
    // Running statistics
    float surprise_ema_ = 0.0f;
    float coherence_ema_ = 0.0f;
    float ema_alpha_ = 0.1f;
};

// ============================================================================
// Channel 3: Evolutionary Feedback
// ============================================================================

struct EvolutionaryFeedback {
    std::chrono::high_resolution_clock::time_point timestamp;
    
    // Performance metrics over time window
    float prediction_accuracy;            // % of correct predictions
    float energy_efficiency;              // Energy used / outcomes achieved
    float learning_speed;                 // Rate of improvement
    float exploration_vs_exploitation;    // Balance metric
    
    // Long-term trends
    float accuracy_trend;                 // Improving / declining
    float novelty_seeking;                // How much new info explored
    
    EvolutionaryFeedback() : prediction_accuracy(0.0f), energy_efficiency(0.0f),
                            learning_speed(0.0f), exploration_vs_exploitation(0.5f),
                            accuracy_trend(0.0f), novelty_seeking(0.5f) {}
};

class EvolutionaryFeedbackChannel {
public:
    EvolutionaryFeedbackChannel();
    
    /**
     * Accumulate performance metrics
     */
    void record_performance(float prediction_accuracy,
                           float energy_used,
                           float outcomes_achieved,
                           float novelty_encountered);
    
    /**
     * Compute rolling fitness over last N cycles
     * Returns smoothed fitness score
     */
    float compute_rolling_fitness(size_t window_size = 100) const;
    
    /**
     * Determine which genome parameters should mutate
     * Only mutate parameters correlated with fitness variance
     */
    struct MutationSuggestion {
        std::vector<std::string> parameters_to_mutate;
        std::vector<float> mutation_strengths;
        float confidence;  // How confident in these suggestions
    };
    
    MutationSuggestion suggest_mutations() const;
    
    /**
     * Check if genome should evolve
     * Returns true if enough data accumulated and variance is high
     */
    bool should_trigger_evolution() const;
    
    /**
     * Get current evolutionary fitness
     */
    EvolutionaryFeedback get_current_fitness() const;
    
    /**
     * Reset after evolution cycle
     */
    void reset_after_evolution();
    
private:
    std::deque<EvolutionaryFeedback> fitness_history_;
    static constexpr size_t MAX_HISTORY = 1000;
    static constexpr size_t MIN_SAMPLES_FOR_EVOLUTION = 100;
    
    // Current accumulation
    float total_predictions_ = 0.0f;
    float correct_predictions_ = 0.0f;
    float total_energy_used_ = 0.0f;
    float total_outcomes_ = 0.0f;
    float total_novelty_ = 0.0f;
    size_t sample_count_ = 0;
    
    // Parameter correlation tracking
    // Maps parameter name -> correlation with fitness
    std::unordered_map<std::string, float> parameter_fitness_correlation_;
    
    void update_parameter_correlations();
};

// ============================================================================
// Unified Feedback Manager
// ============================================================================

class FeedbackManager {
public:
    FeedbackManager();
    
    // Access to channels
    SensoryFeedbackChannel& sensory() { return sensory_channel_; }
    CognitiveFeedbackChannel& cognitive() { return cognitive_channel_; }
    EvolutionaryFeedbackChannel& evolutionary() { return evolutionary_channel_; }
    
    /**
     * Process all feedback channels for one tick
     * Coordinates between channels
     */
    void process_tick();
    
    /**
     * Get overall system health
     */
    struct SystemHealth {
        float sensory_accuracy;      // How well actions match outcomes
        float cognitive_coherence;   // How stable are predictions
        float evolutionary_fitness;  // Long-term performance trend
        float overall_health;        // Combined metric
    };
    
    SystemHealth get_system_health() const;
    
private:
    SensoryFeedbackChannel sensory_channel_;
    CognitiveFeedbackChannel cognitive_channel_;
    EvolutionaryFeedbackChannel evolutionary_channel_;
};

} // namespace cognitive_field
} // namespace melvin

#endif // THREE_CHANNEL_FEEDBACK_H

