/**
 * @file intent_classifier.h
 * @brief Embedding-based query intent classification
 * 
 * Classifies queries into reasoning intents:
 * - DEFINE: "what is X"
 * - LOCATE: "where is X"
 * - CAUSE: "why does X"
 * - COMPARE: "difference between X and Y"
 * - ANALOGY: "X is to Y as A is to ?"
 * - REFLECT: "how do you know X"
 * 
 * Uses embedding similarity, not rule-based parsing
 */

#ifndef MELVIN_INTENT_CLASSIFIER_H
#define MELVIN_INTENT_CLASSIFIER_H

#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>

namespace melvin {
namespace language {

/**
 * @brief Reasoning intent types
 */
enum class ReasoningIntent {
    DEFINE,      // Definition queries: "what is X", "define X"
    LOCATE,      // Location queries: "where is X", "location of X"
    CAUSE,       // Causal queries: "why does X", "what causes X"
    COMPARE,     // Comparison: "difference between X and Y", "X vs Y"
    ANALOGY,     // Analogical: "X is to Y as A is to ?", "like X but for Y"
    REFLECT,     // Meta-cognitive: "how do you know", "why do you think"
    PROCESS,     // Procedural: "how to X", "steps to Y"
    TEMPORAL,    // Time-based: "when did X", "history of Y"
    UNKNOWN      // Cannot classify
};

/**
 * @brief Edge type preferences for each intent
 */
enum class EdgeType {
    EXACT,
    RELATED,
    HAS_PROPERTY,
    IS_A,
    PART_OF,
    HAS_CAPITAL,
    LOCATED_IN,
    CAUSES,
    PRECEDES,
    ENABLES,
    USED_FOR,
    OPPOSITE_OF
};

/**
 * @brief Intent-specific reasoning strategy
 */
struct ReasoningStrategy {
    ReasoningIntent intent;
    std::unordered_map<EdgeType, float> edge_weights;  // Which edges to prefer
    float max_path_length;    // How deep to search
    float temperature;         // Exploration vs exploitation
    bool require_bidirectional; // Must find paths in both directions
    
    ReasoningStrategy() :
        intent(ReasoningIntent::UNKNOWN),
        max_path_length(5.0f),
        temperature(1.0f),
        require_bidirectional(false)
    {}
};

/**
 * @brief Embedding-based intent classifier
 */
class IntentClassifier {
public:
    IntentClassifier();
    ~IntentClassifier() = default;
    
    /**
     * @brief Infer intent from query
     * 
     * Uses embedding similarity to learned intent prototypes
     * Not rule-based string matching
     */
    ReasoningIntent infer_intent(
        const std::vector<float>& query_embedding,
        const std::vector<std::string>& tokens
    );
    
    /**
     * @brief Get reasoning strategy for intent
     * 
     * Returns edge type weights and search parameters
     */
    ReasoningStrategy get_strategy(ReasoningIntent intent) const;
    
    /**
     * @brief Extract key entities from query
     * 
     * Identifies subject, object, predicate
     */
    struct QueryEntities {
        std::vector<std::string> subjects;
        std::vector<std::string> objects;
        std::vector<std::string> predicates;
        std::vector<std::string> modifiers;
    };
    
    QueryEntities extract_entities(
        const std::vector<std::string>& tokens,
        ReasoningIntent intent
    );
    
    /**
     * @brief Check if token is a stop word
     */
    bool is_stop_word(const std::string& token) const;
    
    /**
     * @brief Get content words (filter stop words)
     */
    std::vector<std::string> get_content_words(
        const std::vector<std::string>& tokens
    ) const;
    
private:
    // Intent prototypes (learned embeddings)
    std::unordered_map<ReasoningIntent, std::vector<float>> intent_prototypes_;
    
    // Reasoning strategies
    std::unordered_map<ReasoningIntent, ReasoningStrategy> strategies_;
    
    // Stop words
    std::unordered_set<std::string> stop_words_;
    
    // Question words for heuristic boost
    std::unordered_map<std::string, ReasoningIntent> question_word_hints_;
    
    void initialize_prototypes();
    void initialize_strategies();
    void initialize_stop_words();
    
    float cosine_similarity(
        const std::vector<float>& a,
        const std::vector<float>& b
    ) const;
    
    ReasoningIntent classify_by_keywords(
        const std::vector<std::string>& tokens
    ) const;
};

/**
 * @brief Simple tokenizer helper
 */
std::vector<std::string> tokenize(const std::string& text);

/**
 * @brief Compute simple embedding from tokens
 * (For now - later will use real embedding model)
 */
std::vector<float> compute_simple_embedding(
    const std::vector<std::string>& tokens
);

} // namespace language
} // namespace melvin

#endif // MELVIN_INTENT_CLASSIFIER_H

