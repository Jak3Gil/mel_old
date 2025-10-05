#pragma once

#include "melvin_types.h"
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <functional>

namespace melvin {

// Forward declaration
class Storage;

/**
 * InputConcept - Represents a parsed input concept
 */
struct InputConcept {
    std::string text;                    // Original text
    std::vector<std::string> tokens;     // Tokenized words
    std::vector<NodeID> node_ids;        // Corresponding graph node IDs
    float confidence = 1.0f;            // Parsing confidence
    uint64_t timestamp = 0;             // When this concept was created
    
    InputConcept() = default;
    InputConcept(const std::string& t) : text(t) {}
};

/**
 * PerceptionEngine - Transforms raw input into conceptual nodes
 * 
 * Responsibilities:
 * - Tokenization and lemmatization
 * - Node creation/lookup in persistent memory
 * - Concept formation from tokens
 * - Confidence scoring
 */
class PerceptionEngine {
public:
    PerceptionEngine(std::shared_ptr<Storage> storage);
    ~PerceptionEngine() = default;

    // Core perception methods
    InputConcept parse_input(const std::string& raw_input);
    std::vector<InputConcept> parse_batch(const std::vector<std::string>& inputs);
    
    // Tokenization
    std::vector<std::string> tokenize(const std::string& text);
    std::string lemmatize(const std::string& word);
    
    // Node management
    NodeID get_or_create_node(const std::string& text);
    NodeID find_node(const std::string& text);
    
    // Concept formation
    InputConcept form_concept(const std::vector<std::string>& tokens);
    float compute_confidence(const InputConcept& input_concept);
    
    // Configuration
    void set_tokenizer(std::function<std::vector<std::string>(const std::string&)> tokenizer);
    void set_lemmatizer(std::function<std::string(const std::string&)> lemmatizer);
    
    // Statistics
    size_t get_concept_count() const;
    size_t get_node_count() const;
    void reset_statistics();

private:
    std::shared_ptr<Storage> storage_;
    
    // Tokenization and lemmatization
    std::function<std::vector<std::string>(const std::string&)> tokenizer_;
    std::function<std::string(const std::string&)> lemmatizer_;
    
    // Caching
    std::unordered_map<std::string, NodeID> text_to_node_cache_;
    std::unordered_map<std::string, std::string> lemmatization_cache_;
    
    // Statistics
    std::atomic<size_t> concept_count_;
    std::atomic<size_t> node_count_;
    
    // Internal methods
    std::vector<std::string> default_tokenize(const std::string& text);
    std::string default_lemmatize(const std::string& word);
    void update_caches(const InputConcept& input_concept);
    bool is_valid_token(const std::string& token);
};

} // namespace melvin
