#pragma once

#include "types.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>

namespace melvin {
namespace tokenizer {

/**
 * 🔤 Tokenizer - Bridge Between Text and Concept Nodes
 * 
 * Implements byte-pair encoding (BPE) for sub-word tokenization.
 * Maps between:
 * - Text strings ↔ Token IDs (uint32_t)
 * - Token IDs ↔ Concept nodes (NodeID)
 * 
 * Key Features:
 * - Sub-word tokenization for better vocabulary coverage
 * - Bidirectional mapping to concept graph
 * - Special tokens for control (<BOS>, <EOS>, <PAD>, <UNK>)
 */
class Tokenizer {
public:
    struct Config {
        size_t vocab_size = 50000;            // Total vocabulary size
        size_t max_token_length = 100;        // Max characters per token
        bool lowercase = false;               // Lowercase all input
        bool add_prefix_space = true;         // Add space before tokens
        std::string unk_token = "<UNK>";      // Unknown token
        std::string bos_token = "<BOS>";      // Beginning of sequence
        std::string eos_token = "<EOS>";      // End of sequence
        std::string pad_token = "<PAD>";      // Padding token
        bool verbose = false;                 // Logging
        
        Config();
    };
    
    explicit Tokenizer(const Config& config = Config());
    ~Tokenizer();
    
    // ========================================================================
    // TEXT ↔ TOKENS
    // ========================================================================
    
    /**
     * Encode text to token IDs
     */
    std::vector<uint32_t> encode(const std::string& text) const;
    
    /**
     * Decode token IDs back to text
     */
    std::string decode(const std::vector<uint32_t>& tokens) const;
    
    /**
     * Encode with special tokens (BOS/EOS)
     */
    std::vector<uint32_t> encode_with_special(const std::string& text) const;
    
    /**
     * Tokenize into pieces (for debugging)
     */
    std::vector<std::string> tokenize(const std::string& text) const;
    
    // ========================================================================
    // TOKENS ↔ CONCEPT NODES
    // ========================================================================
    
    /**
     * Map token ID to concept node ID
     * Creates node if it doesn't exist
     */
    NodeID token_to_node(uint32_t token_id);
    
    /**
     * Map concept node ID to token ID
     * Returns UNK token if not found
     */
    uint32_t node_to_token(NodeID node_id) const;
    
    /**
     * Check if mapping exists
     */
    bool has_token_mapping(uint32_t token_id) const;
    bool has_node_mapping(NodeID node_id) const;
    
    // ========================================================================
    // VOCABULARY MANAGEMENT
    // ========================================================================
    
    /**
     * Build vocabulary from text corpus
     * Uses BPE algorithm to learn subword units
     */
    void train_from_corpus(const std::vector<std::string>& corpus);
    
    /**
     * Add token to vocabulary
     */
    uint32_t add_token(const std::string& token);
    
    /**
     * Get token string by ID
     */
    std::string get_token(uint32_t token_id) const;
    
    /**
     * Get token ID by string
     */
    uint32_t get_token_id(const std::string& token) const;
    
    /**
     * Get vocabulary size
     */
    size_t vocab_size() const;
    
    // ========================================================================
    // SPECIAL TOKENS
    // ========================================================================
    
    uint32_t unk_token_id() const;
    uint32_t bos_token_id() const;
    uint32_t eos_token_id() const;
    uint32_t pad_token_id() const;
    
    // ========================================================================
    // PERSISTENCE
    // ========================================================================
    
    /**
     * Save vocabulary and mappings to file
     */
    bool save(const std::string& path) const;
    
    /**
     * Load vocabulary and mappings from file
     */
    bool load(const std::string& path);
    
    // ========================================================================
    // STATISTICS
    // ========================================================================
    
    struct Stats {
        size_t total_tokens = 0;
        size_t mapped_tokens = 0;        // Tokens with node mappings
        size_t unknown_count = 0;        // UNK token usage
        float compression_ratio = 0.0f;   // chars/token ratio
    };
    
    Stats get_stats() const;
    void print_stats() const;
    
private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace tokenizer
} // namespace melvin

