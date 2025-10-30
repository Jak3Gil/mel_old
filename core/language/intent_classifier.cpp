/**
 * @file intent_classifier.cpp
 * @brief Implementation of intent classifier
 */

#include "intent_classifier.h"
#include <algorithm>
#include <sstream>
#include <cctype>
#include <cmath>

namespace melvin {
namespace language {

std::vector<std::string> tokenize(const std::string& text) {
    std::vector<std::string> tokens;
    std::stringstream ss(text);
    std::string word;
    
    while (ss >> word) {
        // Remove punctuation and convert to lowercase
        word.erase(std::remove_if(word.begin(), word.end(), ::ispunct), word.end());
        std::transform(word.begin(), word.end(), word.begin(), ::tolower);
        
        if (!word.empty()) {
            tokens.push_back(word);
        }
    }
    
    return tokens;
}

std::vector<float> compute_simple_embedding(const std::vector<std::string>& tokens) {
    // Simple hash-based embedding for now
    // TODO: Replace with real embedding model
    std::vector<float> embedding(128, 0.0f);
    
    for (const auto& token : tokens) {
        size_t hash = std::hash<std::string>{}(token);
        for (size_t i = 0; i < 128; i++) {
            embedding[i] += std::sin(static_cast<float>(hash + i) * 0.01f);
        }
    }
    
    // Normalize
    float norm = 0.0f;
    for (float val : embedding) {
        norm += val * val;
    }
    norm = std::sqrt(norm);
    
    if (norm > 1e-6f) {
        for (float& val : embedding) {
            val /= norm;
        }
    }
    
    return embedding;
}

IntentClassifier::IntentClassifier() {
    initialize_prototypes();
    initialize_strategies();
    initialize_stop_words();
}

ReasoningIntent IntentClassifier::infer_intent(
    const std::vector<float>& query_embedding,
    const std::vector<std::string>& tokens
) {
    // First try keyword-based heuristic for speed
    ReasoningIntent keyword_intent = classify_by_keywords(tokens);
    if (keyword_intent != ReasoningIntent::UNKNOWN) {
        return keyword_intent;
    }
    
    // Fall back to embedding similarity
    float best_similarity = -1.0f;
    ReasoningIntent best_intent = ReasoningIntent::UNKNOWN;
    
    for (const auto& [intent, prototype] : intent_prototypes_) {
        float sim = cosine_similarity(query_embedding, prototype);
        if (sim > best_similarity) {
            best_similarity = sim;
            best_intent = intent;
        }
    }
    
    // Require minimum confidence
    if (best_similarity < 0.3f) {
        return ReasoningIntent::UNKNOWN;
    }
    
    return best_intent;
}

ReasoningStrategy IntentClassifier::get_strategy(ReasoningIntent intent) const {
    auto it = strategies_.find(intent);
    if (it != strategies_.end()) {
        return it->second;
    }
    return ReasoningStrategy();  // Default strategy
}

IntentClassifier::QueryEntities IntentClassifier::extract_entities(
    const std::vector<std::string>& tokens,
    ReasoningIntent intent
) {
    QueryEntities entities;
    
    // Filter stop words
    auto content = get_content_words(tokens);
    
    // Simple entity extraction based on position and intent
    if (intent == ReasoningIntent::DEFINE && !content.empty()) {
        // "what is X" -> X is subject
        entities.subjects.push_back(content.back());
    }
    else if (intent == ReasoningIntent::LOCATE && !content.empty()) {
        // "where is X" -> X is subject
        entities.subjects.push_back(content.back());
    }
    else if (intent == ReasoningIntent::CAUSE && content.size() >= 2) {
        // "why does X Y" -> X is subject, Y is predicate
        entities.subjects.push_back(content[content.size() - 2]);
        entities.predicates.push_back(content.back());
    }
    else if (intent == ReasoningIntent::COMPARE && content.size() >= 2) {
        // "X vs Y" or "difference between X and Y"
        entities.subjects.push_back(content[0]);
        entities.objects.push_back(content.back());
    }
    else {
        // Default: all content words are subjects
        entities.subjects = content;
    }
    
    return entities;
}

bool IntentClassifier::is_stop_word(const std::string& token) const {
    return stop_words_.count(token) > 0;
}

std::vector<std::string> IntentClassifier::get_content_words(
    const std::vector<std::string>& tokens
) const {
    std::vector<std::string> content;
    for (const auto& token : tokens) {
        if (!is_stop_word(token)) {
            content.push_back(token);
        }
    }
    return content;
}

void IntentClassifier::initialize_prototypes() {
    // Create learned prototypes from typical queries
    // In real implementation, these would be learned from data
    
    std::vector<std::string> define_examples = {"what", "is", "define", "meaning", "definition"};
    std::vector<std::string> locate_examples = {"where", "location", "place", "situated", "found"};
    std::vector<std::string> cause_examples = {"why", "cause", "reason", "because", "due"};
    std::vector<std::string> compare_examples = {"difference", "compare", "versus", "vs", "between"};
    std::vector<std::string> analogy_examples = {"like", "similar", "analogy", "comparable", "as"};
    std::vector<std::string> reflect_examples = {"how", "know", "sure", "certain", "think"};
    std::vector<std::string> process_examples = {"how", "steps", "procedure", "method", "way"};
    std::vector<std::string> temporal_examples = {"when", "time", "date", "history", "ago"};
    
    intent_prototypes_[ReasoningIntent::DEFINE] = compute_simple_embedding(define_examples);
    intent_prototypes_[ReasoningIntent::LOCATE] = compute_simple_embedding(locate_examples);
    intent_prototypes_[ReasoningIntent::CAUSE] = compute_simple_embedding(cause_examples);
    intent_prototypes_[ReasoningIntent::COMPARE] = compute_simple_embedding(compare_examples);
    intent_prototypes_[ReasoningIntent::ANALOGY] = compute_simple_embedding(analogy_examples);
    intent_prototypes_[ReasoningIntent::REFLECT] = compute_simple_embedding(reflect_examples);
    intent_prototypes_[ReasoningIntent::PROCESS] = compute_simple_embedding(process_examples);
    intent_prototypes_[ReasoningIntent::TEMPORAL] = compute_simple_embedding(temporal_examples);
}

void IntentClassifier::initialize_strategies() {
    // DEFINE strategy
    {
        ReasoningStrategy s;
        s.intent = ReasoningIntent::DEFINE;
        s.edge_weights[EdgeType::HAS_PROPERTY] = 0.9f;
        s.edge_weights[EdgeType::IS_A] = 0.8f;
        s.edge_weights[EdgeType::RELATED] = 0.5f;
        s.max_path_length = 3.0f;
        strategies_[ReasoningIntent::DEFINE] = s;
    }
    
    // LOCATE strategy
    {
        ReasoningStrategy s;
        s.intent = ReasoningIntent::LOCATE;
        s.edge_weights[EdgeType::LOCATED_IN] = 0.95f;
        s.edge_weights[EdgeType::PART_OF] = 0.7f;
        s.edge_weights[EdgeType::HAS_CAPITAL] = 0.9f;
        s.max_path_length = 4.0f;
        strategies_[ReasoningIntent::LOCATE] = s;
    }
    
    // CAUSE strategy
    {
        ReasoningStrategy s;
        s.intent = ReasoningIntent::CAUSE;
        s.edge_weights[EdgeType::CAUSES] = 0.95f;
        s.edge_weights[EdgeType::PRECEDES] = 0.7f;
        s.edge_weights[EdgeType::ENABLES] = 0.8f;
        s.max_path_length = 6.0f;
        strategies_[ReasoningIntent::CAUSE] = s;
    }
    
    // COMPARE strategy
    {
        ReasoningStrategy s;
        s.intent = ReasoningIntent::COMPARE;
        s.edge_weights[EdgeType::OPPOSITE_OF] = 0.9f;
        s.edge_weights[EdgeType::IS_A] = 0.7f;
        s.edge_weights[EdgeType::HAS_PROPERTY] = 0.6f;
        s.max_path_length = 5.0f;
        s.require_bidirectional = true;
        strategies_[ReasoningIntent::COMPARE] = s;
    }
    
    // ANALOGY strategy
    {
        ReasoningStrategy s;
        s.intent = ReasoningIntent::ANALOGY;
        s.edge_weights[EdgeType::RELATED] = 0.8f;
        s.edge_weights[EdgeType::IS_A] = 0.7f;
        s.edge_weights[EdgeType::USED_FOR] = 0.7f;
        s.max_path_length = 5.0f;
        strategies_[ReasoningIntent::ANALOGY] = s;
    }
    
    // Default for others
    ReasoningStrategy default_s;
    default_s.edge_weights[EdgeType::RELATED] = 0.7f;
    default_s.max_path_length = 5.0f;
    strategies_[ReasoningIntent::REFLECT] = default_s;
    strategies_[ReasoningIntent::PROCESS] = default_s;
    strategies_[ReasoningIntent::TEMPORAL] = default_s;
    strategies_[ReasoningIntent::UNKNOWN] = default_s;
}

void IntentClassifier::initialize_stop_words() {
    stop_words_ = {
        "a", "an", "the", "is", "are", "was", "were", "be", "been", "being",
        "have", "has", "had", "do", "does", "did", "will", "would", "should",
        "could", "may", "might", "must", "can", "of", "in", "on", "at", "to",
        "for", "with", "by", "from", "about", "as", "into", "through", "during",
        "before", "after", "above", "below", "between", "under", "again", "further",
        "then", "once", "here", "there", "all", "both", "each", "few", "more",
        "most", "other", "some", "such", "no", "nor", "not", "only", "own",
        "same", "so", "than", "too", "very", "s", "t", "just", "don", "now"
    };
    
    // Question words for heuristic hints
    question_word_hints_["what"] = ReasoningIntent::DEFINE;
    question_word_hints_["where"] = ReasoningIntent::LOCATE;
    question_word_hints_["why"] = ReasoningIntent::CAUSE;
    question_word_hints_["when"] = ReasoningIntent::TEMPORAL;
    question_word_hints_["how"] = ReasoningIntent::PROCESS;
}

float IntentClassifier::cosine_similarity(
    const std::vector<float>& a,
    const std::vector<float>& b
) const {
    if (a.size() != b.size() || a.empty()) return 0.0f;
    
    float dot = 0.0f;
    float norm_a = 0.0f;
    float norm_b = 0.0f;
    
    for (size_t i = 0; i < a.size(); i++) {
        dot += a[i] * b[i];
        norm_a += a[i] * a[i];
        norm_b += b[i] * b[i];
    }
    
    float denom = std::sqrt(norm_a) * std::sqrt(norm_b);
    return (denom > 1e-6f) ? (dot / denom) : 0.0f;
}

ReasoningIntent IntentClassifier::classify_by_keywords(
    const std::vector<std::string>& tokens
) const {
    if (tokens.empty()) return ReasoningIntent::UNKNOWN;
    
    // Check first few tokens for question words
    for (size_t i = 0; i < std::min(size_t(3), tokens.size()); i++) {
        auto it = question_word_hints_.find(tokens[i]);
        if (it != question_word_hints_.end()) {
            // Special case: "how to" suggests PROCESS
            if (tokens[i] == "how" && i + 1 < tokens.size() && tokens[i + 1] == "to") {
                return ReasoningIntent::PROCESS;
            }
            // "how do you know" suggests REFLECT
            else if (tokens[i] == "how" && tokens.size() > 3) {
                for (size_t j = i + 1; j < tokens.size(); j++) {
                    if (tokens[j] == "know" || tokens[j] == "think") {
                        return ReasoningIntent::REFLECT;
                    }
                }
            }
            return it->second;
        }
    }
    
    // Check for comparison keywords
    for (const auto& token : tokens) {
        if (token == "difference" || token == "compare" || token == "vs" || token == "versus") {
            return ReasoningIntent::COMPARE;
        }
    }
    
    return ReasoningIntent::UNKNOWN;
}

} // namespace language
} // namespace melvin

