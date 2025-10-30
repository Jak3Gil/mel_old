#ifndef REFLECTIVE_CONTROLLER_H
#define REFLECTIVE_CONTROLLER_H

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <chrono>
#include "../fields/activation_field_unified.h"
#include "../feedback/three_channel_feedback.h"

namespace melvin {
namespace metacognition {

/**
 * @brief Reflective Controller - Meta-Cognition Layer
 * 
 * Melvin thinks about his own thinking.
 * Monitors internal metrics and spawns self-queries.
 * Stores results as ReflectionNodes feeding the genome layer.
 */

// ============================================================================
// Reflection Node - Meta-Cognitive Concept
// ============================================================================

struct ReflectionNode {
    int node_id;
    std::string query;  // "What changed?" "Why did I fail?" etc.
    std::string answer;  // Derived from graph analysis
    
    // Metrics that triggered this reflection
    float prediction_error;
    float coherence;
    float novelty;
    
    // Impact on genome
    std::unordered_map<std::string, float> suggested_gene_changes;
    
    std::chrono::high_resolution_clock::time_point timestamp;
    
    ReflectionNode(int id, const std::string& q)
        : node_id(id), query(q), prediction_error(0), coherence(0), novelty(0),
          timestamp(std::chrono::high_resolution_clock::now()) {}
};

// ============================================================================
// Reflective Triggers
// ============================================================================

struct ReflectiveTrigger {
    enum class Type {
        HIGH_ERROR,      // Prediction error spike
        LOW_COHERENCE,   // Field instability
        HIGH_NOVELTY,    // Unexpected input
        GOAL_FAILURE,    // Failed to achieve goal
        UNEXPECTED_REWARD,  // Surprise reward/punishment
        LEARNING_PLATEAU,   // No improvement over time
        CONTEXT_SHIFT    // Sudden change in environment
    };
    
    Type type;
    float magnitude;
    std::vector<int> relevant_nodes;  // Which nodes are involved?
    
    ReflectiveTrigger(Type t, float m) : type(t), magnitude(m) {}
};

// ============================================================================
// Self-Query System
// ============================================================================

class SelfQuery {
public:
    // Generate query based on trigger
    static std::string generate_query(const ReflectiveTrigger& trigger);
    
    // Answer query using graph analysis
    static std::string answer_query(
        const std::string& query,
        const fields::UnifiedActivationField& field,
        const feedback::FeedbackCoordinator& feedback);
    
private:
    // Query templates
    static std::unordered_map<ReflectiveTrigger::Type, std::vector<std::string>> query_templates_;
    
    // Analysis methods
    static std::string analyze_error_pattern(
        const fields::UnifiedActivationField& field,
        const std::vector<int>& nodes);
    
    static std::string analyze_coherence_drop(
        const fields::UnifiedActivationField& field);
    
    static std::string analyze_novelty(
        const fields::UnifiedActivationField& field,
        const std::vector<int>& nodes);
    
    static std::string analyze_goal_failure(
        const fields::UnifiedActivationField& field,
        const feedback::FeedbackCoordinator& feedback);
};

// ============================================================================
// Reflective Controller
// ============================================================================

class ReflectiveController {
public:
    ReflectiveController(fields::UnifiedActivationField& field,
                        feedback::FeedbackCoordinator& feedback);
    
    // Monitor metrics and trigger reflections
    void tick(float dt);
    
    // Check if reflection should be triggered
    std::vector<ReflectiveTrigger> check_triggers();
    
    // Spawn a reflection
    ReflectionNode spawn_reflection(const ReflectiveTrigger& trigger);
    
    // Apply reflection results to genome
    void apply_reflections_to_genome();
    
    // Get recent reflections
    std::vector<ReflectionNode> get_recent_reflections(size_t count = 10);
    
    // Get all reflections
    const std::vector<ReflectionNode>& get_all_reflections() const {
        return reflections_;
    }
    
    // Statistics
    struct Stats {
        size_t total_reflections;
        float avg_reflection_impact;  // How much reflections change genome
        std::unordered_map<ReflectiveTrigger::Type, int> trigger_counts;
    };
    Stats get_stats() const;
    
private:
    fields::UnifiedActivationField& field_;
    feedback::FeedbackCoordinator& feedback_;
    
    // Reflection storage
    std::vector<ReflectionNode> reflections_;
    int next_reflection_id_ = 2000000;  // Start from high number
    
    // Trigger thresholds
    float error_threshold_ = 0.5f;
    float coherence_threshold_ = 0.3f;
    float novelty_threshold_ = 0.8f;
    
    // Tracking for trigger detection
    std::vector<float> recent_errors_;
    std::vector<float> recent_coherence_;
    std::vector<float> recent_rewards_;
    size_t history_window_ = 100;
    
    // Reflection rate limiting
    std::chrono::high_resolution_clock::time_point last_reflection_;
    float min_reflection_interval_ = 1.0f;  // At most once per second
    
    // Helper methods
    bool should_trigger_error_reflection();
    bool should_trigger_coherence_reflection();
    bool should_trigger_novelty_reflection();
    bool should_trigger_plateau_reflection();
    
    float compute_trend(const std::vector<float>& values);
    float compute_variance(const std::vector<float>& values);
};

} // namespace metacognition
} // namespace melvin

#endif // REFLECTIVE_CONTROLLER_H

