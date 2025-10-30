/**
 * @file answer_synthesizer.h
 * @brief Generate natural language answers from scored nodes
 * 
 * Template-based answer generation:
 * - DEFINE: "X is Y"
 * - LOCATE: "X is located in Y"
 * - CAUSE: "X causes Y because Z"
 * - COMPARE: "X differs from Y in Z"
 */

#ifndef MELVIN_ANSWER_SYNTHESIZER_H
#define MELVIN_ANSWER_SYNTHESIZER_H

#include <string>
#include <vector>
#include <unordered_map>
#include "semantic_scorer.h"
#include "core/language/intent_classifier.h"

namespace melvin {
namespace reasoning {

/**
 * @brief Answer synthesizer
 * 
 * Generates natural language answers based on:
 * - Query intent
 * - Scored nodes
 * - Reasoning paths
 */
class AnswerSynthesizer {
public:
    AnswerSynthesizer();
    ~AnswerSynthesizer() = default;
    
    /**
     * @brief Generate answer from scored nodes
     */
    std::string generate(
        const std::vector<ScoredNode>& scored_nodes,
        const std::unordered_map<int, std::string>& id_to_word,
        melvin::language::ReasoningIntent intent,
        const std::vector<std::string>& query_tokens,
        const std::vector<int>& query_node_ids
    );
    
    /**
     * @brief Generate reasoning explanation
     * 
     * Shows how the answer was derived
     */
    std::string generate_explanation(
        const ScoredNode& answer_node,
        const std::unordered_map<int, std::string>& id_to_word
    );
    
private:
    // Intent-specific generation
    std::string generate_definition(
        const std::vector<ScoredNode>& nodes,
        const std::unordered_map<int, std::string>& id_to_word,
        const std::vector<std::string>& query_tokens
    );
    
    std::string generate_location(
        const std::vector<ScoredNode>& nodes,
        const std::unordered_map<int, std::string>& id_to_word,
        const std::vector<std::string>& query_tokens
    );
    
    std::string generate_cause(
        const std::vector<ScoredNode>& nodes,
        const std::unordered_map<int, std::string>& id_to_word,
        const std::vector<std::string>& query_tokens
    );
    
    std::string generate_comparison(
        const std::vector<ScoredNode>& nodes,
        const std::unordered_map<int, std::string>& id_to_word,
        const std::vector<std::string>& query_tokens
    );
    
    std::string generate_generic(
        const std::vector<ScoredNode>& nodes,
        const std::unordered_map<int, std::string>& id_to_word,
        const std::vector<std::string>& query_tokens
    );
    
    // Helpers
    std::string extract_subject(
        const std::vector<std::string>& query_tokens,
        melvin::language::ReasoningIntent intent
    );
    
    std::string capitalize_first(const std::string& str);
    
    bool is_query_node(int node_id, const std::vector<int>& query_node_ids);
};

} // namespace reasoning
} // namespace melvin

#endif // MELVIN_ANSWER_SYNTHESIZER_H

