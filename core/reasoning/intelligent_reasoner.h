/**
 * @file intelligent_reasoner.h
 * @brief Complete intelligent reasoning system
 * 
 * Integrates:
 * - Intent classification
 * - Semantic-biased graph traversal
 * - Dynamic scoring
 * - Metrics tracking
 * - Answer synthesis
 * - Continuous learning
 */

#ifndef MELVIN_INTELLIGENT_REASONER_H
#define MELVIN_INTELLIGENT_REASONER_H

#include <string>
#include <vector>
#include <unordered_map>
#include "core/language/intent_classifier.h"
#include "core/metrics/reasoning_metrics.h"
#include "core/evolution/dynamic_genome.h"
#include "core/metacognition/reflection_controller_dynamic.h"
#include "semantic_scorer.h"
#include "answer_synthesizer.h"

namespace melvin {
namespace reasoning {

/**
 * @brief Complete reasoning result
 */
struct ReasoningResult {
    std::string answer;
    std::string explanation;
    melvin::language::ReasoningIntent intent;
    
    // Metrics
    float confidence;
    float coherence;
    float novelty;
    
    // Top scored nodes
    std::vector<ScoredNode> top_nodes;
    
    ReasoningResult() :
        intent(melvin::language::ReasoningIntent::UNKNOWN),
        confidence(0.0f),
        coherence(0.0f),
        novelty(0.0f)
    {}
};

/**
 * @brief Intelligent reasoning engine
 * 
 * Complete pipeline from query to natural language answer
 */
class IntelligentReasoner {
public:
    IntelligentReasoner();
    ~IntelligentReasoner() = default;
    
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
     * @brief Answer a natural language query
     * 
     * Complete pipeline:
     * 1. Tokenize query
     * 2. Classify intent
     * 3. Extract entities
     * 4. Activate query nodes
     * 5. Spread activation (semantic-biased)
     * 6. Score all activated nodes
     * 7. Synthesize natural language answer
     * 8. Update metrics and learn
     */
    ReasoningResult answer(const std::string& query);
    
    /**
     * @brief Provide feedback for learning
     * 
     * Call after user confirms/corrects answer
     */
    void feedback(bool correct);
    
    /**
     * @brief Get current reasoning metrics
     */
    const melvin::metrics::ReasoningMetrics& metrics() const {
        return metrics_tracker_.current();
    }
    
    /**
     * @brief Get current reasoning mode
     */
    melvin::metacognition::ReasoningMode current_mode() const {
        return reflection_controller_.current_mode();
    }
    
    /**
     * @brief Get mode switch statistics
     */
    melvin::metacognition::ReflectionController::ModeStats mode_stats() const {
        return reflection_controller_.get_stats();
    }
    
    /**
     * @brief Save learned parameters
     */
    void save_genome(const std::string& filepath);
    void load_genome(const std::string& filepath);
    
private:
    // Components
    melvin::language::IntentClassifier intent_classifier_;
    melvin::metrics::ReasoningMetricsTracker metrics_tracker_;
    melvin::evolution::DynamicGenome genome_;
    melvin::metacognition::ReflectionController reflection_controller_;
    SemanticScorer scorer_;
    AnswerSynthesizer synthesizer_;
    
    // Knowledge graph
    std::unordered_map<int, std::vector<std::pair<int, float>>> graph_;
    std::unordered_map<int, std::vector<float>> embeddings_;
    std::unordered_map<std::string, int> word_to_id_;
    std::unordered_map<int, std::string> id_to_word_;
    
    // Working memory for learning
    ReasoningResult last_result_;
    
    // Reasoning steps
    std::vector<int> activate_query_nodes(
        const std::vector<std::string>& tokens
    );
    
    void spread_activation(
        const std::vector<int>& seed_nodes,
        melvin::language::ReasoningStrategy strategy,
        std::unordered_map<int, float>& activations,
        std::unordered_map<int, std::vector<int>>& paths
    );
    
    std::vector<ScoredNode> score_activated_nodes(
        const std::unordered_map<int, float>& activations,
        const std::unordered_map<int, std::vector<int>>& paths,
        const std::vector<float>& query_embedding
    );
    
    void update_metrics(
        const std::vector<int>& active_nodes,
        const std::unordered_map<int, float>& activations,
        const std::vector<ScoredNode>& scored_nodes
    );
};

} // namespace reasoning
} // namespace melvin

#endif // MELVIN_INTELLIGENT_REASONER_H

