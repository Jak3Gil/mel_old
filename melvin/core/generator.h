#pragma once

#include "types.h"
#include "storage.h"
#include "hybrid_predictor.h"
#include "tokenizer.h"
#include <string>
#include <vector>
#include <memory>

namespace melvin {
namespace generator {

/**
 * 📝 Generator - Graph-Constrained Text Generation
 * 
 * High-level interface for text generation using the hybrid predictor.
 * Provides multiple decoding strategies and graph-constrained output.
 * 
 * KEY ADVANTAGE: Can't hallucinate facts not in the graph!
 */
class Generator {
public:
    enum class DecodingStrategy {
        GREEDY,              // Always pick highest probability
        BEAM_SEARCH,         // Keep top-k sequences
        NUCLEUS_SAMPLING,    // Sample from top-p probability mass (best for creativity)
        GRAPH_CONSTRAINED    // Only output tokens valid in graph (best for accuracy)
    };
    
    struct Config {
        DecodingStrategy strategy = DecodingStrategy::NUCLEUS_SAMPLING;
        int max_length = 256;             // Maximum tokens to generate
        float temperature = 0.8f;         // Sampling temperature (lower = more deterministic)
        float top_p = 0.9f;               // Nucleus sampling threshold
        int beam_width = 5;               // For beam search
        float length_penalty = 1.0f;      // Prefer longer/shorter sequences
        bool graph_constrained = true;    // Enforce graph validity
        float min_graph_validity = 0.1f;  // Minimum graph support required
        bool verbose = false;             // Logging
        
        Config();
    };
    
    explicit Generator(
        hybrid::HybridPredictor* predictor,
        tokenizer::Tokenizer* tokenizer,
        Storage* storage,
        const Config& config = Config()
    );
    ~Generator();
    
    // ========================================================================
    // TEXT GENERATION
    // ========================================================================
    
    /**
     * Generate text from a prompt
     */
    std::string generate(const std::string& prompt);
    
    /**
     * Generate with explicit concept constraints
     * Forces output to include certain concepts from graph
     */
    std::string generate_with_concepts(
        const std::string& prompt,
        const std::vector<std::string>& required_concepts
    );
    
    /**
     * Generate answer to a question
     * Uses graph to find supporting evidence
     */
    std::string answer_question(
        const std::string& question,
        bool include_reasoning = false
    );
    
    /**
     * Complete a partial sentence
     */
    std::string complete(const std::string& partial_text);
    
    // ========================================================================
    // GRAPH-GUIDED GENERATION
    // ========================================================================
    
    /**
     * Generate following a reasoning path
     * Ensures output follows graph connections
     */
    std::string generate_from_path(
        const ReasoningPath& path,
        const std::string& prompt = ""
    );
    
    /**
     * Generate explanation for a concept
     * Traverses graph to build explanation
     */
    std::string explain_concept(const std::string& concept);
    
    // ========================================================================
    // VALIDATION
    // ========================================================================
    
    /**
     * Check if generated text is supported by graph
     * Returns (is_valid, confidence, supporting_nodes)
     */
    struct ValidationResult {
        bool is_valid;
        float confidence;
        std::vector<NodeID> supporting_nodes;
        std::string explanation;
    };
    
    ValidationResult validate_text(const std::string& text);
    
    /**
     * Verify a specific claim against the graph
     */
    bool verify_claim(const std::string& claim);
    
    // ========================================================================
    // DECODING STRATEGIES
    // ========================================================================
    
    /**
     * Greedy decoding: always pick highest probability
     */
    std::vector<uint32_t> decode_greedy(
        const std::vector<uint32_t>& prompt_tokens
    );
    
    /**
     * Beam search: maintain top-k hypotheses
     */
    std::vector<uint32_t> decode_beam_search(
        const std::vector<uint32_t>& prompt_tokens
    );
    
    /**
     * Nucleus (top-p) sampling: sample from top probability mass
     */
    std::vector<uint32_t> decode_nucleus(
        const std::vector<uint32_t>& prompt_tokens
    );
    
    /**
     * Graph-constrained: only allow tokens valid in graph
     */
    std::vector<uint32_t> decode_graph_constrained(
        const std::vector<uint32_t>& prompt_tokens
    );
    
    // ========================================================================
    // CONFIGURATION
    // ========================================================================
    
    void set_config(const Config& config);
    const Config& get_config() const;
    
    void set_strategy(DecodingStrategy strategy);
    DecodingStrategy get_strategy() const;
    
    // ========================================================================
    // STATISTICS
    // ========================================================================
    
    struct Stats {
        uint64_t generations = 0;
        uint64_t tokens_generated = 0;
        float avg_length = 0.0f;
        float avg_confidence = 0.0f;
        uint64_t graph_constrained_generations = 0;
        uint64_t hallucinations_prevented = 0;  // Times graph blocked invalid output
    };
    
    Stats get_stats() const;
    void reset_stats();
    void print_stats() const;
    
private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace generator
} // namespace melvin

