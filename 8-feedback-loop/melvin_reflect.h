#pragma once

#include "melvin_graph.h"
#include "melvin_vision.h"
#include "melvin_reasoning.h"
#include <vector>
#include <string>

namespace melvin {
namespace uca {

// ============================================================================
// BIOLOGICAL ANALOG: Predictive Coding (Hierarchical Prediction)
// ============================================================================

/**
 * Prediction types
 */
enum PredictionType {
    SENSORY_PREDICTION,      // What I expect to see/hear next
    OBJECT_PREDICTION,       // What objects I expect
    EVENT_PREDICTION,        // What will happen next
    OUTCOME_PREDICTION       // Result of action
};

/**
 * Prediction structure
 */
struct Prediction {
    PredictionType type;
    std::string what;              // Predicted content
    float confidence;              // How sure am I?
    double when;                   // Expected time
    std::vector<uint64_t> basis;   // Graph nodes supporting prediction
    
    Prediction() : type(SENSORY_PREDICTION), confidence(0), when(0) {}
};

/**
 * Prediction error (mismatch between expected vs observed)
 */
struct PredictionError {
    Prediction prediction;
    std::string actual;            // What actually happened
    float error_magnitude;         // How wrong was I?
    double timestamp;
    
    // Consequences
    bool triggers_learning;        // Update model?
    bool triggers_curiosity;       // Explore more?
    bool triggers_attention;       // Focus on this?
    
    PredictionError() : error_magnitude(0), timestamp(0),
                       triggers_learning(false), triggers_curiosity(false),
                       triggers_attention(false) {}
};

// ============================================================================
// MELVIN REFLECTION - Predictive Coding & Error-Driven Learning
// ============================================================================

/**
 * MelvinReflection: Implements predictive coding framework
 * 
 * Core principle: Brain is constantly predicting, learning from errors
 * 
 * Loop:
 * 1. Generate predictions (forward model)
 * 2. Compare to observations
 * 3. Compute prediction errors
 * 4. Update model weights (Hebbian + error backprop)
 * 5. Trigger curiosity/attention for high errors
 * 
 * Biological parallels:
 * - Forward model → Cerebellar prediction
 * - Error detection → Anterior cingulate cortex (ACC)
 * - Model update → Synaptic plasticity (LTP/LTD)
 * - Curiosity → Dopaminergic surprise signals
 */
class MelvinReflection {
public:
    struct Config {
        // Prediction generation
        bool enable_sensory_prediction;   // Predict next visual/audio
        bool enable_object_prediction;    // Predict object presence
        bool enable_event_prediction;     // Predict sequences
        bool enable_outcome_prediction;   // Predict action results
        
        // Learning from errors
        float learning_rate;              // How fast to update (default: 0.1)
        float error_threshold;            // Min error to trigger learning (default: 0.3)
        bool enable_hebbian_update;       // Update co-occurrence weights
        bool enable_leap_discovery;       // Discover new shortcuts
        
        // Curiosity
        float curiosity_gain;             // Error → curiosity multiplier (default: 2.0)
        bool trigger_exploration;         // High error → explore more
        
        Config() : enable_sensory_prediction(true),
                   enable_object_prediction(true),
                   enable_event_prediction(true),
                   enable_outcome_prediction(false),
                   learning_rate(0.1f), error_threshold(0.3f),
                   enable_hebbian_update(true), enable_leap_discovery(true),
                   curiosity_gain(2.0f), trigger_exploration(true) {}
    };
    
    MelvinReflection(MelvinGraph* graph, const Config& config = Config());
    ~MelvinReflection();
    
    // ========================================================================
    // PREDICTION GENERATION (Forward Model)
    // ========================================================================
    
    /**
     * Generate predictions about next sensory input
     * Based on current context and temporal patterns
     */
    std::vector<Prediction> predict_next_sensory(
        const std::vector<VisualNode>& current_objects,
        double current_time
    );
    
    /**
     * Predict what objects should appear next
     * Based on scene context and typical co-occurrences
     */
    std::vector<Prediction> predict_objects(
        const std::vector<std::string>& context_objects
    );
    
    /**
     * Predict sequence continuation
     * Given A→B→C, predict D
     */
    std::vector<Prediction> predict_sequence(
        const std::vector<uint64_t>& sequence
    );
    
    /**
     * Predict outcome of action
     * "If I do X, what happens?"
     */
    std::vector<Prediction> predict_outcome(
        const std::string& action,
        const std::string& context
    );
    
    // ========================================================================
    // ERROR DETECTION & PROCESSING
    // ========================================================================
    
    /**
     * Compare predictions to observations
     * Returns prediction errors for learning
     */
    std::vector<PredictionError> compute_errors(
        const std::vector<Prediction>& predictions,
        const std::vector<VisualNode>& observations
    );
    
    /**
     * Process errors: update graph, trigger curiosity
     */
    void process_errors(
        const std::vector<PredictionError>& errors,
        MelvinGraph& graph
    );
    
    /**
     * Get high-error items (surprising observations)
     */
    std::vector<PredictionError> get_surprising_errors(float threshold = 0.7f) const;
    
    // ========================================================================
    // MODEL UPDATE (Learning)
    // ========================================================================
    
    /**
     * Update graph weights based on prediction errors
     * Strengthens correct predictions, weakens incorrect ones
     */
    void update_graph_weights(
        const std::vector<PredictionError>& errors,
        MelvinGraph& graph
    );
    
    /**
     * Discover new patterns from errors
     * If consistently wrong, maybe missing a connection
     */
    void discover_patterns(
        const std::vector<PredictionError>& errors,
        MelvinGraph& graph
    );
    
    /**
     * Replay past predictions to consolidate learning
     * Sleep-like memory consolidation
     */
    void replay_and_consolidate(MelvinGraph& graph, int num_replays = 10);
    
    // ========================================================================
    // CURIOSITY SYSTEM
    // ========================================================================
    
    /**
     * Compute curiosity score from prediction errors
     * High error = high curiosity
     */
    float compute_curiosity(const PredictionError& error);
    
    /**
     * Generate exploration goals based on errors
     * "I need to learn more about X"
     */
    std::vector<std::string> generate_exploration_goals(
        const std::vector<PredictionError>& errors
    );
    
    // ========================================================================
    // FORWARD SIMULATION (Mental Imagery)
    // ========================================================================
    
    /**
     * Simulate hypothetical scenario
     * "What would happen if...?"
     */
    struct Simulation {
        std::string scenario;
        std::vector<Prediction> predicted_states;
        float plausibility;
    };
    
    Simulation simulate_scenario(const std::string& scenario, int num_steps = 5);
    
    /**
     * Compare simulations (counterfactual reasoning)
     * "What's the difference between scenario A and B?"
     */
    std::string compare_simulations(const Simulation& a, const Simulation& b);
    
    // ========================================================================
    // STATE & STATISTICS
    // ========================================================================
    
    struct ReflectionStats {
        uint64_t total_predictions;
        uint64_t total_errors;
        float avg_error_magnitude;
        uint64_t high_curiosity_events;
        uint64_t model_updates;
        uint64_t patterns_discovered;
    };
    
    ReflectionStats get_stats() const { return stats_; }
    void print_stats() const;
    
    /**
     * Get current active predictions
     */
    const std::vector<Prediction>& get_active_predictions() const {
        return active_predictions_;
    }
    
private:
    // ========================================================================
    // HELPER FUNCTIONS
    // ========================================================================
    
    // Generate prediction from graph patterns
    Prediction generate_prediction_from_graph(
        uint64_t context_node,
        PredictionType type
    );
    
    // Compute error magnitude
    float compute_error_magnitude(
        const Prediction& pred,
        const std::string& actual
    );
    
    // Check if error should trigger learning
    bool should_trigger_learning(const PredictionError& error);
    
    // ========================================================================
    // STATE
    // ========================================================================
    
    MelvinGraph* graph_;
    Config config_;
    
    // Active predictions waiting to be validated
    std::vector<Prediction> active_predictions_;
    
    // Recent errors (for learning)
    std::vector<PredictionError> recent_errors_;
    
    // Statistics
    ReflectionStats stats_;
};

} // namespace uca
} // namespace melvin


