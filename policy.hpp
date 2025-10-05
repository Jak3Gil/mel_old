#pragma once

#include "melvin_types.h"
#include "scoring.hpp"
#include "beam.hpp"
#include <string>
#include <vector>
#include <unordered_map>
#include <regex>
#include <unordered_set>

namespace melvin {

// Forward declarations
struct Query;

// Query intent types
enum class QueryIntent {
    DEFINE,         // "What is X?", "Define Y", "Meaning of Z"
    WHY,            // "Why does X happen?", "How does Y work?"
    COMPARE,        // "Compare X and Y", "Which is better?"
    CAUSAL,         // "What causes X?", "What leads to Y?"
    TEMPORAL,       // "What happens after X?", "What comes before Y?"
    GENERAL,        // General question, no specific pattern
    UNKNOWN         // Cannot determine intent
};

// Query complexity levels
enum class QueryComplexity {
    SIMPLE,         // Single concept, direct question
    MODERATE,       // Multiple concepts, some reasoning required
    COMPLEX,        // Multi-step reasoning, synthesis required
    UNKNOWN         // Cannot determine complexity
};

// Query classification result
struct QueryClassification {
    QueryIntent intent;
    QueryComplexity complexity;
    double confidence;
    std::vector<std::string> key_terms;
    std::vector<std::string> entities;
    std::string reasoning;
    
    QueryClassification() : intent(QueryIntent::UNKNOWN), complexity(QueryComplexity::UNKNOWN), confidence(0.0) {}
};

// Beam search parameters for different query types
struct QueryPolicy {
    BeamParams beam_params;
    ScoringWeights scoring_weights;
    std::vector<Rel> preferred_relations;
    size_t max_depth_multiplier;
    double confidence_threshold;
    
    QueryPolicy() : max_depth_multiplier(1), confidence_threshold(0.5) {}
    QueryPolicy(const QueryPolicy&) = default;
    QueryPolicy& operator=(const QueryPolicy&) = default;
};

// Query intent classifier
class QueryIntentClassifier {
private:
    std::unordered_map<std::string, QueryIntent> intent_keywords_;
    std::unordered_map<std::string, double> keyword_weights_;
    std::vector<std::regex> intent_patterns_;
    
public:
    QueryIntentClassifier();
    
    // Classify query intent
    QueryClassification classify(const Query& query) const;
    
    // Classify from text string
    QueryClassification classify_text(const std::string& query_text) const;
    
    // Get beam search policy for query type
    QueryPolicy get_policy_for_intent(QueryIntent intent) const;
    
    // Configuration
    void add_intent_keyword(const std::string& keyword, QueryIntent intent, double weight = 1.0);
    void add_intent_pattern(const std::string& pattern, QueryIntent intent);
    
private:
    // Pattern matching
    QueryIntent match_patterns(const std::string& query) const;
    QueryIntent match_keywords(const std::vector<std::string>& tokens) const;
    
    // Complexity analysis
    QueryComplexity analyze_complexity(const std::string& query, const std::vector<std::string>& entities) const;
    
    // Entity extraction
    std::vector<std::string> extract_entities(const std::string& query) const;
    std::vector<std::string> extract_key_terms(const std::string& query) const;
    
    // Initialization
    void initialize_keywords();
    void initialize_patterns();
};

// Query complexity analyzer
class QueryComplexityAnalyzer {
private:
    std::unordered_set<std::string> complexity_indicators_;
    std::unordered_set<std::string> logical_connectors_;
    
public:
    QueryComplexityAnalyzer();
    
    // Analyze query complexity
    QueryComplexity analyze_complexity(const std::string& query) const;
    
    // Get complexity score (0.0 = simple, 1.0 = complex)
    double get_complexity_score(const std::string& query) const;
    
    // Configuration
    void add_complexity_indicator(const std::string& indicator);
    void add_logical_connector(const std::string& connector);
    
private:
    // Complexity factors
    int count_entities(const std::string& query) const;
    int count_relations(const std::string& query) const;
    bool has_conditional_logic(const std::string& query) const;
    bool has_comparison_logic(const std::string& query) const;
    
    // Initialization
    void initialize_indicators();
    void initialize_connectors();
};

// Entity extractor
class EntityExtractor {
private:
    std::unordered_set<std::string> stop_words_;
    std::vector<std::regex> entity_patterns_;
    
public:
    EntityExtractor();
    
    // Extract entities from query
    std::vector<std::string> extract_entities(const std::string& query) const;
    
    // Extract key terms (non-stop words)
    std::vector<std::string> extract_key_terms(const std::string& query) const;
    
    // Check if term is a stop word
    bool is_stop_word(const std::string& term) const;
    
    // Configuration
    void add_stop_word(const std::string& word);
    void add_entity_pattern(const std::string& pattern);
    
private:
    // Text processing
public:
    std::vector<std::string> tokenize(const std::string& text) const;
private:
    std::string normalize_term(const std::string& term) const;
    
    // Initialization
    void initialize_stop_words();
    void initialize_entity_patterns();
};

// Policy manager
class PolicyManager {
private:
    QueryIntentClassifier classifier_;
    QueryComplexityAnalyzer complexity_analyzer_;
    EntityExtractor entity_extractor_;
    
    std::unordered_map<QueryIntent, QueryPolicy> intent_policies_;
    
public:
    PolicyManager();
    
    // Get complete policy for query
    QueryPolicy get_policy(const Query& query) const;
    QueryPolicy get_policy(const std::string& query_text) const;
    
    // Get policy for specific intent
    QueryPolicy get_policy_for_intent(QueryIntent intent) const;
    
    // Update policy for intent
    void set_policy_for_intent(QueryIntent intent, const QueryPolicy& policy);
    
    // Classify query
    QueryClassification classify_query(const Query& query) const;
    QueryClassification classify_query(const std::string& query_text) const;
    
    // Configuration
    void set_default_policies();
    void customize_policy(QueryIntent intent, const BeamParams& beam_params, 
                         const ScoringWeights& scoring_weights);
    
private:
    // Policy initialization
    void initialize_default_policies();
    QueryPolicy create_define_policy() const;
    QueryPolicy create_why_policy() const;
    QueryPolicy create_compare_policy() const;
    QueryPolicy create_causal_policy() const;
    QueryPolicy create_temporal_policy() const;
    QueryPolicy create_general_policy() const;
};

// Query preprocessing
class QueryPreprocessor {
private:
    EntityExtractor entity_extractor_;
    
public:
    QueryPreprocessor();
    
    // Preprocess query for better classification
    Query preprocess_query(const std::string& raw_query) const;
    
    // Normalize query text
    std::string normalize_query(const std::string& query) const;
    
    // Extract query components
    std::vector<std::string> extract_tokens(const std::string& query) const;
    std::vector<float> compute_token_embeddings(const std::vector<std::string>& tokens) const;
    
    // Configuration
    void set_entity_extractor(const EntityExtractor& extractor) { entity_extractor_ = extractor; }
    
private:
    // Text normalization
    std::string lowercase(const std::string& text) const;
    std::string remove_punctuation(const std::string& text) const;
    std::string normalize_whitespace(const std::string& text) const;
    
    // Embedding computation (simplified)
    std::vector<float> compute_simple_embedding(const std::string& token) const;
};

// Query routing system
class QueryRouter {
private:
    PolicyManager policy_manager_;
    QueryPreprocessor preprocessor_;
    
public:
    QueryRouter();
    
    // Route query to appropriate processing pipeline
    struct RoutingResult {
        QueryPolicy policy;
        QueryClassification classification;
        Query processed_query;
        std::vector<NodeID> suggested_start_nodes;
    };
    
    RoutingResult route_query(const std::string& raw_query, Store* store) const;
    
    // Configuration
    void set_policy_manager(const PolicyManager& manager) { policy_manager_ = manager; }
    void set_preprocessor(const QueryPreprocessor& preprocessor) { preprocessor_ = preprocessor; }
    
private:
    // Start node selection
    std::vector<NodeID> select_start_nodes(const Query& query, QueryIntent intent, Store* store) const;
    NodeID find_best_start_node(const std::vector<std::string>& entities, Store* store) const;
    
    // Policy adaptation
    QueryPolicy adapt_policy_for_complexity(const QueryPolicy& base_policy, 
                                           QueryComplexity complexity) const;
};

// Utility functions
namespace PolicyUtils {
    // Convert intent to string
    std::string intent_to_string(QueryIntent intent);
    
    // Convert complexity to string
    std::string complexity_to_string(QueryComplexity complexity);
    
    // Check if query requires multi-hop reasoning
    bool requires_multi_hop_reasoning(QueryIntent intent, QueryComplexity complexity);
    
    // Get recommended beam width for query type
    size_t get_recommended_beam_width(QueryIntent intent, QueryComplexity complexity);
    
    // Get recommended max depth for query type
    size_t get_recommended_max_depth(QueryIntent intent, QueryComplexity complexity);
    
    // Check if query is likely to have a definitive answer
    bool has_definitive_answer(QueryIntent intent);
    
    // Get confidence adjustment for query type
    double get_confidence_adjustment(QueryIntent intent);
}

} // namespace melvin
