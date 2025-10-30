#ifndef REFLECTIVE_CONTROLLER_H
#define REFLECTIVE_CONTROLLER_H

#include <vector>
#include <string>
#include <unordered_map>
#include <chrono>
#include <deque>

namespace melvin {
namespace cognitive_field {

/**
 * Reflective Controller
 * 
 * Meta-cognition layer that monitors system performance and spawns
 * self-queries like "what changed?", "why did that fail?", "what should I focus on?"
 * 
 * This enables MELVIN to think about its own thinking.
 */

// ============================================================================
// Reflection Types
// ============================================================================

enum class ReflectionType {
    PERFORMANCE_ANALYSIS,    // "Why am I failing at X?"
    CHANGE_DETECTION,        // "What changed?"
    FOCUS_SELECTION,         // "What should I pay attention to?"
    GOAL_EVALUATION,         // "Am I making progress toward goal?"
    KNOWLEDGE_GAP,           // "What don't I know?"
    STRATEGY_SELECTION,      // "Should I explore or exploit?"
    ERROR_DIAGNOSIS,         // "Why was I wrong?"
    LEARNING_PROGRESS        // "Am I improving?"
};

// ============================================================================
// Reflection Query
// ============================================================================

struct ReflectionQuery {
    ReflectionType type;
    std::chrono::high_resolution_clock::time_point timestamp;
    
    // Query context
    std::string query_text;              // Natural language question
    std::vector<int> relevant_nodes;     // Nodes involved in the question
    float urgency;                       // How important is this question
    
    // Triggers
    float prediction_error_trigger;      // What prediction error spawned this
    float surprise_trigger;              // What surprise level spawned this
    float coherence_trigger;             // What coherence loss spawned this
    
    ReflectionQuery() : type(ReflectionType::PERFORMANCE_ANALYSIS),
                       urgency(0.5f), prediction_error_trigger(0.0f),
                       surprise_trigger(0.0f), coherence_trigger(0.0f) {}
};

// ============================================================================
// Reflection Answer
// ============================================================================

struct ReflectionAnswer {
    ReflectionQuery query;
    std::chrono::high_resolution_clock::time_point answer_time;
    
    // Answer content
    std::vector<int> answer_nodes;       // Nodes that form the answer
    std::vector<float> node_relevance;   // How relevant each node is
    float confidence;                    // Confidence in this answer
    
    // Actionable insights
    std::vector<std::string> insights;   // Text insights (for logging)
    std::vector<int> nodes_to_boost;     // Attention adjustments
    std::vector<int> nodes_to_suppress;
    bool trigger_consolidation;          // Should we consolidate now?
    bool trigger_evolution;              // Should genome evolve?
    
    ReflectionAnswer() : confidence(0.0f), trigger_consolidation(false),
                        trigger_evolution(false) {}
};

// ============================================================================
// Meta-Cognitive Metrics
// ============================================================================

struct MetaCognitiveMetrics {
    // Performance awareness
    float perceived_prediction_accuracy;  // How well do I think I'm doing?
    float actual_prediction_accuracy;     // How well am I actually doing?
    float metacognitive_calibration;      // Difference (awareness quality)
    
    // Knowledge awareness
    float known_knowledge;                // What I know I know
    float unknown_knowledge;              // What I know I don't know
    float unknown_unknowns;               // Estimate of blind spots
    
    // Learning awareness
    float learning_rate_estimate;         // How fast am I learning?
    float plateau_detection;              // Am I stuck?
    float breakthrough_detection;         // Did I just learn something big?
    
    // Strategy awareness
    float current_exploration_rate;       // How much am I exploring?
    float optimal_exploration_rate;       // How much should I explore?
    float strategy_effectiveness;         // Is my strategy working?
    
    MetaCognitiveMetrics()
        : perceived_prediction_accuracy(0.0f), actual_prediction_accuracy(0.0f),
          metacognitive_calibration(0.0f), known_knowledge(0.0f),
          unknown_knowledge(0.0f), unknown_unknowns(0.5f),
          learning_rate_estimate(0.0f), plateau_detection(0.0f),
          breakthrough_detection(0.0f), current_exploration_rate(0.5f),
          optimal_exploration_rate(0.5f), strategy_effectiveness(0.5f) {}
};

// ============================================================================
// Reflective Controller
// ============================================================================

class ReflectiveController {
public:
    ReflectiveController();
    
    // ========================================================================
    // Monitoring (called every tick)
    // ========================================================================
    
    /**
     * Monitor system metrics and spawn reflection queries when needed
     */
    void monitor(float prediction_error,
                 float coherence,
                 float surprise,
                 float energy_level,
                 size_t active_nodes);
    
    /**
     * Get pending reflection queries
     */
    std::vector<ReflectionQuery> get_pending_queries() const;
    
    /**
     * Answer a reflection query
     * This triggers graph search, reasoning, and pattern matching
     */
    ReflectionAnswer answer_query(const ReflectionQuery& query,
                                  const std::vector<int>& context_nodes,
                                  const std::unordered_map<int, float>& node_activations);
    
    /**
     * Submit answer and take action
     */
    void submit_answer(const ReflectionAnswer& answer);
    
    // ========================================================================
    // Meta-Cognitive Analysis
    // ========================================================================
    
    /**
     * Compute current meta-cognitive metrics
     */
    MetaCognitiveMetrics compute_metacognitive_metrics() const;
    
    /**
     * Detect performance plateaus
     * Returns true if learning has stalled
     */
    bool detect_plateau(size_t lookback_window = 100) const;
    
    /**
     * Detect breakthroughs
     * Returns true if sudden performance improvement
     */
    bool detect_breakthrough(size_t lookback_window = 50) const;
    
    /**
     * Estimate knowledge gaps
     * Returns concepts with high uncertainty
     */
    std::vector<std::pair<int, float>> estimate_knowledge_gaps(size_t top_k = 10) const;
    
    // ========================================================================
    // Self-Query Generation
    // ========================================================================
    
    /**
     * Generate "what changed?" query
     */
    ReflectionQuery generate_change_detection_query() const;
    
    /**
     * Generate "why did I fail?" query
     */
    ReflectionQuery generate_error_diagnosis_query(
        int predicted_node, int actual_node, float error) const;
    
    /**
     * Generate "what should I focus on?" query
     */
    ReflectionQuery generate_focus_selection_query() const;
    
    /**
     * Generate "am I improving?" query
     */
    ReflectionQuery generate_learning_progress_query() const;
    
    // ========================================================================
    // Reflection History
    // ========================================================================
    
    /**
     * Get recent reflection answers
     */
    std::vector<ReflectionAnswer> get_recent_reflections(size_t count) const;
    
    /**
     * Find past reflections of a specific type
     */
    std::vector<ReflectionAnswer> find_reflections_by_type(
        ReflectionType type, size_t count) const;
    
    /**
     * Get reflection insights as text (for logging/debugging)
     */
    std::vector<std::string> get_recent_insights(size_t count) const;
    
    // ========================================================================
    // Statistics
    // ========================================================================
    
    struct Stats {
        size_t total_queries_generated;
        size_t total_queries_answered;
        float avg_answer_confidence;
        float metacognitive_calibration;
        size_t consolidations_triggered;
        size_t evolutions_triggered;
    };
    
    Stats get_stats() const;
    
private:
    // Query queue
    std::deque<ReflectionQuery> pending_queries_;
    static constexpr size_t MAX_PENDING_QUERIES = 20;
    
    // Answer history
    std::deque<ReflectionAnswer> answer_history_;
    static constexpr size_t MAX_ANSWER_HISTORY = 200;
    
    // Performance tracking for meta-cognition
    struct PerformanceSnapshot {
        std::chrono::high_resolution_clock::time_point timestamp;
        float prediction_error;
        float coherence;
        float surprise;
        float energy;
        size_t active_nodes;
    };
    std::deque<PerformanceSnapshot> performance_history_;
    static constexpr size_t MAX_PERFORMANCE_HISTORY = 1000;
    
    // Thresholds for spawning queries
    float high_error_threshold_ = 0.5f;
    float low_coherence_threshold_ = 0.3f;
    float high_surprise_threshold_ = 0.7f;
    float plateau_threshold_ = 0.05f;  // Change in performance
    
    // Statistics
    Stats stats_;
    
    // Helper functions
    bool should_spawn_query(ReflectionType type) const;
    float compute_query_urgency(ReflectionType type,
                               float error, float coherence, float surprise) const;
    
    std::vector<int> find_relevant_nodes_for_query(const ReflectionQuery& query,
                                                    const std::vector<int>& context) const;
    
    std::vector<std::string> generate_insights(const ReflectionQuery& query,
                                               const std::vector<int>& answer_nodes) const;
};

} // namespace cognitive_field
} // namespace melvin

#endif // REFLECTIVE_CONTROLLER_H

