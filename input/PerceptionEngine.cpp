#include "PerceptionEngine.hpp"
#include "../storage.h"
#include <algorithm>
#include <cctype>
#include <sstream>
#include <chrono>
#include <regex>

namespace melvin {

PerceptionEngine::PerceptionEngine(std::shared_ptr<Storage> storage) 
    : storage_(storage), concept_count_(0), node_count_(0) {
    
    // Set default tokenizer and lemmatizer
    tokenizer_ = [this](const std::string& text) { return default_tokenize(text); };
    lemmatizer_ = [this](const std::string& word) { return default_lemmatize(word); };
}

InputConcept PerceptionEngine::parse_input(const std::string& raw_input) {
    // Tokenize the input
    std::vector<std::string> tokens = tokenizer_(raw_input);
    
    // Form concept from tokens
    InputConcept input_concept = form_concept(tokens);
    input_concept.text = raw_input;
    input_concept.timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    
    // Compute confidence
    input_concept.confidence = compute_confidence(input_concept);
    
    // Update statistics
    concept_count_++;
    
    return input_concept;
}

std::vector<InputConcept> PerceptionEngine::parse_batch(const std::vector<std::string>& inputs) {
    std::vector<InputConcept> concepts;
    concepts.reserve(inputs.size());
    
    for (const auto& input : inputs) {
        concepts.push_back(parse_input(input));
    }
    
    return concepts;
}

std::vector<std::string> PerceptionEngine::tokenize(const std::string& text) {
    return tokenizer_(text);
}

std::string PerceptionEngine::lemmatize(const std::string& word) {
    return lemmatizer_(word);
}

NodeID PerceptionEngine::get_or_create_node(const std::string& text) {
    // Check cache first
    auto it = text_to_node_cache_.find(text);
    if (it != text_to_node_cache_.end()) {
        return it->second;
    }
    
    // Create new node via storage
    NodeID node_id = {};
    // Note: In a real implementation, this would call storage_->create_node()
    // For now, we'll generate a simple hash-based ID
    std::hash<std::string> hasher;
    size_t hash = hasher(text);
    std::memcpy(node_id.data(), &hash, std::min(sizeof(hash), node_id.size()));
    
    // Cache the result
    text_to_node_cache_[text] = node_id;
    node_count_++;
    
    return node_id;
}

NodeID PerceptionEngine::find_node(const std::string& text) {
    auto it = text_to_node_cache_.find(text);
    if (it != text_to_node_cache_.end()) {
        return it->second;
    }
    
    // Return zero ID if not found
    return {};
}

InputConcept PerceptionEngine::form_concept(const std::vector<std::string>& tokens) {
    InputConcept input_concept;
    input_concept.tokens = tokens;
    
    // Create or find nodes for each token
    for (const std::string& token : tokens) {
        if (is_valid_token(token)) {
            NodeID node_id = get_or_create_node(token);
            input_concept.node_ids.push_back(node_id);
        }
    }
    
    return input_concept;
}

float PerceptionEngine::compute_confidence(const InputConcept& input_concept) {
    if (input_concept.tokens.empty()) return 0.0f;
    
    float confidence = 1.0f;
    
    // Reduce confidence for very short or very long concepts
    size_t token_count = input_concept.tokens.size();
    if (token_count == 1) {
        confidence *= 0.8f;  // Single tokens are less confident
    } else if (token_count > 10) {
        confidence *= 0.9f;  // Very long concepts are less confident
    }
    
    // Reduce confidence for unknown tokens
    float known_ratio = static_cast<float>(input_concept.node_ids.size()) / token_count;
    confidence *= known_ratio;
    
    return std::max(0.0f, std::min(1.0f, confidence));
}

void PerceptionEngine::set_tokenizer(std::function<std::vector<std::string>(const std::string&)> tokenizer) {
    tokenizer_ = tokenizer;
}

void PerceptionEngine::set_lemmatizer(std::function<std::string(const std::string&)> lemmatizer) {
    lemmatizer_ = lemmatizer;
}

size_t PerceptionEngine::get_concept_count() const {
    return concept_count_.load();
}

size_t PerceptionEngine::get_node_count() const {
    return node_count_.load();
}

void PerceptionEngine::reset_statistics() {
    concept_count_.store(0);
    node_count_.store(0);
}

std::vector<std::string> PerceptionEngine::default_tokenize(const std::string& text) {
    std::vector<std::string> tokens;
    std::istringstream iss(text);
    std::string word;
    
    while (iss >> word) {
        // Basic preprocessing
        std::transform(word.begin(), word.end(), word.begin(), ::tolower);
        
        // Remove punctuation (simple approach)
        word.erase(std::remove_if(word.begin(), word.end(), 
            [](char c) { return std::ispunct(c); }), word.end());
        
        if (!word.empty()) {
            tokens.push_back(word);
        }
    }
    
    return tokens;
}

std::string PerceptionEngine::default_lemmatize(const std::string& word) {
    // Check cache first
    auto it = lemmatization_cache_.find(word);
    if (it != lemmatization_cache_.end()) {
        return it->second;
    }
    
    std::string lemma = word;
    
    // Simple lemmatization rules
    if (word.length() > 3) {
        // Remove common suffixes
        if (word.ends_with("ing")) {
            lemma = word.substr(0, word.length() - 3);
        } else if (word.ends_with("ed")) {
            lemma = word.substr(0, word.length() - 2);
        } else if (word.ends_with("s") && word.length() > 4) {
            lemma = word.substr(0, word.length() - 1);
        } else if (word.ends_with("ly")) {
            lemma = word.substr(0, word.length() - 2);
        }
    }
    
    // Cache the result
    lemmatization_cache_[word] = lemma;
    
    return lemma;
}

void PerceptionEngine::update_caches(const InputConcept& input_concept) {
    // Update node cache for each token
    for (size_t i = 0; i < input_concept.tokens.size() && i < input_concept.node_ids.size(); ++i) {
        text_to_node_cache_[input_concept.tokens[i]] = input_concept.node_ids[i];
    }
}

bool PerceptionEngine::is_valid_token(const std::string& token) {
    if (token.empty()) return false;
    
    // Check if token contains only letters and is reasonable length
    bool has_letter = false;
    for (char c : token) {
        if (std::isalpha(c)) {
            has_letter = true;
        } else if (!std::isalnum(c) && c != '-' && c != '_') {
            return false;  // Invalid character
        }
    }
    
    return has_letter && token.length() <= 50;  // Reasonable length limit
}

} // namespace melvin
