/**
 * @file unified_intelligence.h
 * @brief Single unified pipeline for dynamic intelligence
 * 
 * Integrates all components into one cohesive system:
 * - Intent classification
 * - Semantic traversal
 * - Dynamic scoring
 * - Answer generation
 * - Continuous learning
 * - Autonomous adaptation
 * 
 * All controlled by a single shared genome
 */

#ifndef MELVIN_UNIFIED_INTELLIGENCE_H
#define MELVIN_UNIFIED_INTELLIGENCE_H

#include <string>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <atomic>
#include "core/evolution/dynamic_genome.h"
#include "core/language/intent_classifier.h"
#include "core/metrics/reasoning_metrics.h"
#include "core/metacognition/reflection_controller_dynamic.h"

namespace melvin {
namespace intelligence {

/**
 * @brief Complete reasoning result with all metadata
 */
struct UnifiedResult {
    // Answer
    std::string answer;
    std::string explanation;
    
    // Intent
    language::ReasoningIntent intent;
    language::ReasoningStrategy strategy;
    
    // Metrics
    float confidence;
    float coherence;
    float novelty;
    float semantic_fit;
    
    // Reasoning state
    metacognition::ReasoningMode mode;
    int active_nodes;
    int reasoning_steps;
    std::vector<std::string> reasoning_path;
    
    // Top concepts
    std::vector<std::pair<std::string, float>> top_concepts;
    
    UnifiedResult() :
        intent(language::ReasoningIntent::UNKNOWN),
        confidence(0.0f),
        coherence(0.0f),
        novelty(0.0f),
        semantic_fit(0.0f),
        mode(metacognition::ReasoningMode::EXPLORATORY),
        active_nodes(0),
        reasoning_steps(0)
    {}
};

/**
 * @brief Unified intelligent system
 * 
 * Single pipeline where:
 * 1. All components share one genome
 * 2. Metrics flow between all stages
 * 3. Reflection adapts the entire system
 * 4. Learning updates all parameters simultaneously
 */
class UnifiedIntelligence {
public:
    UnifiedIntelligence();
    ~UnifiedIntelligence() = default;
    
    /**
     * @brief Initialize with knowledge graph
     */
    void initialize(
        const std::unordered_map<int, std::vector<std::pair<int, float>>>& graph,
        const std::unordered_map<int, std::vector<float>>& embeddings,
        const std::unordered_map<std::string, int>& word_to_id,
        const std::unordered_map<int, std::string>& id_to_word
    );
    
    /**
     * @brief Complete unified reasoning pipeline
     * 
     * Single function that does everything:
     * - Classify intent → get strategy
     * - Activate nodes → spread semantically
     * - Score dynamically → rank results
     * - Generate answer → explain reasoning
     * - Update metrics → reflect and adapt
     * - Learn from experience
     */
    UnifiedResult reason(const std::string& query);
    
    /**
     * @brief Learn from feedback
     * 
     * Updates ALL components simultaneously:
     * - Genome parameters (α, β, γ, θ)
     * - Intent profiles
     * - Meta-learning
     * - Mode preferences
     */
    void learn(bool correct);
    
    /**
     * @brief Graph growth: Add new concept to knowledge graph
     * 
     * Creates a new node for a novel concept encountered in experience.
     * Returns the new node ID, or existing ID if concept already exists.
     */
    int add_concept(const std::string& concept, const std::vector<float>& embedding);
    
    /**
     * @brief Graph growth: Add or strengthen edge between concepts
     * 
     * If edge exists, strengthens it (Hebbian learning).
     * If edge doesn't exist, creates it.
     * Returns true if edge was added/strengthened.
     */
    bool strengthen_connection(int from_id, int to_id, float weight_delta = 0.1f);
    
    /**
     * @brief Graph growth: Weaken edge between concepts
     * 
     * Reduces connection strength. If weight drops below threshold, removes edge.
     */
    void weaken_connection(int from_id, int to_id, float weight_delta = 0.05f);
    
    /**
     * @brief Apply Hebbian learning: strengthen edges between co-activated nodes
     * 
     * Called after reasoning to strengthen connections that fired together.
     */
    void apply_hebbian_learning(const std::unordered_map<int, float>& activations, float learning_rate = 0.01f);
    
    /**
     * @brief Get current system state
     */
    const evolution::DynamicGenome& genome() const { return genome_; }
    const metrics::ReasoningMetrics& metrics() const { return current_metrics_; }
    metacognition::ReasoningMode mode() const { return current_mode_; }
    
    /**
     * @brief Save/load learned state
     */
    void save(const std::string& filepath);
    void load(const std::string& filepath);
    
    /**
     * @brief Reset to initial state
     */
    void reset();
    
private:
    // Single shared genome controls everything
    evolution::DynamicGenome genome_;
    
    // All components read from genome
    language::IntentClassifier intent_classifier_;
    metrics::ReasoningMetricsTracker metrics_tracker_;
    metacognition::ReflectionController reflection_controller_;
    
    // Knowledge graph (mutable for growth)
    std::unordered_map<int, std::vector<std::pair<int, float>>> graph_;
    std::unordered_map<int, std::vector<float>> embeddings_;
    std::unordered_map<std::string, int> word_to_id_;
    std::unordered_map<int, std::string> id_to_word_;
    
    // Thread safety for graph mutation
    mutable std::mutex graph_mutex_;
    std::atomic<int> next_node_id_{0};
    
    // Current state
    metrics::ReasoningMetrics current_metrics_;
    metacognition::ReasoningMode current_mode_;
    UnifiedResult last_result_;
    
    // Unified pipeline stages (all use same genome)
    std::vector<std::string> tokenize_and_filter(const std::string& query);
    
    language::ReasoningIntent classify_intent(
        const std::vector<std::string>& tokens,
        const std::vector<float>& query_embedding
    );
    
    language::ReasoningStrategy get_strategy(language::ReasoningIntent intent);
    
    std::vector<int> activate_nodes(const std::vector<std::string>& tokens);
    
    void spread_activation(
        const std::vector<int>& seeds,
        const language::ReasoningStrategy& strategy,
        const std::vector<float>& query_embedding,
        std::unordered_map<int, float>& activations,
        std::unordered_map<int, std::vector<int>>& paths
    );
    
    std::vector<std::pair<int, float>> score_and_rank(
        const std::unordered_map<int, float>& activations,
        const std::unordered_map<int, std::vector<int>>& paths,
        const std::vector<float>& query_embedding
    );
    
    std::string synthesize_answer(
        const std::vector<std::pair<int, float>>& ranked,
        language::ReasoningIntent intent,
        const std::vector<std::string>& query_tokens
    );
    
    void update_metrics(
        const std::unordered_map<int, float>& activations,
        const std::vector<std::pair<int, float>>& ranked
    );
    
    void reflect_and_adapt();
    
    // Helpers
    std::vector<float> compute_embedding(const std::vector<std::string>& tokens);
    float cosine_similarity(const std::vector<float>& a, const std::vector<float>& b);
};

} // namespace intelligence
} // namespace melvin

#endif // MELVIN_UNIFIED_INTELLIGENCE_H

