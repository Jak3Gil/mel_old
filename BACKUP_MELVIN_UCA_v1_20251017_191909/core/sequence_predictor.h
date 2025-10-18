#pragma once

#include "types.h"
#include <vector>
#include <memory>

namespace melvin {
namespace sequence {

/**
 * 🤖 Sequence Predictor - Transformer for Token Prediction
 * 
 * Implements a simplified transformer architecture for next-token prediction.
 * Key innovation: Accepts graph context from GNN to bias predictions!
 * 
 * Architecture:
 * - Multi-head self-attention
 * - Feed-forward layers
 * - Layer normalization
 * - Positional encoding
 * - Graph context injection (NEW!)
 * 
 * Formula: P(next|sequence, graph) = softmax(Transformer(sequence) + GraphBias(graph))
 */
class SequencePredictor {
public:
    struct Config {
        int vocab_size = 50000;       // Vocabulary size
        int d_model = 512;            // Model dimension
        int n_heads = 8;              // Multi-head attention heads
        int n_layers = 6;             // Transformer layers
        int d_ff = 2048;              // Feed-forward dimension
        int context_len = 2048;       // Maximum sequence length
        float dropout = 0.1f;         // Dropout rate
        float learning_rate = 0.0001f; // Learning rate
        bool use_graph_bias = true;   // Enable graph context
        float graph_bias_weight = 0.3f; // Graph influence weight
        bool verbose = false;         // Logging
        
        Config();
    };
    
    explicit SequencePredictor(const Config& config = Config());
    ~SequencePredictor();
    
    // ========================================================================
    // PREDICTION
    // ========================================================================
    
    /**
     * Predict next token probabilities given sequence
     * Returns: probability distribution over vocabulary
     */
    std::vector<float> predict_next(const std::vector<uint32_t>& token_sequence) const;
    
    /**
     * Predict with graph context bias (KEY INNOVATION!)
     * Graph context from GNN biases the prediction
     */
    std::vector<float> predict_next_with_graph(
        const std::vector<uint32_t>& token_sequence,
        const std::vector<NodeID>& graph_context,
        const std::vector<float>& graph_activations
    ) const;
    
    /**
     * Generate tokens autoregressively
     * Uses sampling strategy (greedy, nucleus, etc.)
     */
    std::vector<uint32_t> generate(
        const std::vector<uint32_t>& prefix,
        size_t max_length,
        float temperature = 1.0f,
        float top_p = 0.9f
    ) const;
    
    // ========================================================================
    // TRAINING
    // ========================================================================
    
    /**
     * Train on a batch of sequences
     * Uses teacher forcing and cross-entropy loss
     */
    float train_batch(
        const std::vector<std::vector<uint32_t>>& sequences,
        const std::vector<std::vector<uint32_t>>& targets
    );
    
    /**
     * Train with graph context
     */
    float train_batch_with_graph(
        const std::vector<std::vector<uint32_t>>& sequences,
        const std::vector<std::vector<uint32_t>>& targets,
        const std::vector<std::vector<NodeID>>& graph_contexts,
        const std::vector<std::vector<float>>& graph_activations
    );
    
    /**
     * Compute perplexity on validation set
     */
    float compute_perplexity(
        const std::vector<std::vector<uint32_t>>& sequences
    ) const;
    
    // ========================================================================
    // CONFIGURATION
    // ========================================================================
    
    void set_config(const Config& config);
    const Config& get_config() const;
    
    // ========================================================================
    // PERSISTENCE
    // ========================================================================
    
    /**
     * Save model weights
     */
    bool save(const std::string& path) const;
    
    /**
     * Load model weights
     */
    bool load(const std::string& path);
    
    // ========================================================================
    // STATISTICS
    // ========================================================================
    
    struct Stats {
        uint64_t training_steps = 0;
        float avg_loss = 0.0f;
        float perplexity = 0.0f;
        float graph_bias_contribution = 0.0f;  // How much graph helps
        uint64_t tokens_processed = 0;
    };
    
    Stats get_stats() const;
    void reset_stats();
    void print_stats() const;
    
private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace sequence
} // namespace melvin

