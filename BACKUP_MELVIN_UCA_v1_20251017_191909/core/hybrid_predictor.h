#pragma once

#include "types.h"
#include "storage.h"
#include "gnn_predictor.h"
#include "sequence_predictor.h"
#include "tokenizer.h"
#include <vector>
#include <memory>

namespace melvin {
namespace hybrid {

/**
 * 🔀 Hybrid Predictor - Combines GNN + Transformer
 * 
 * THE KEY INNOVATION: Melvin's advantage over pure LLMs!
 * 
 * Formula: P(next) = α * P_graph(next) + β * P_sequence(next)
 * Where:
 * - P_graph: GNN-based concept prediction (explicit knowledge)
 * - P_sequence: Transformer-based token prediction (fluency)
 * - α, β: Learned or fixed weights
 * 
 * Graph acts as a "fact checker" - can veto hallucinations!
 * 
 * Advantages:
 * 1. Traceable: Can explain predictions via graph paths
 * 2. Grounded: Graph prevents factual errors
 * 3. Fluent: Sequence model ensures natural language
 * 4. Updatable: Add EXACT edge = instant new knowledge
 */
class HybridPredictor {
public:
    struct Config {
        float graph_weight = 0.6f;          // α: Trust graph more for facts
        float sequence_weight = 0.4f;       // β: Sequence for fluency
        bool graph_gates_output = true;     // Graph can veto invalid outputs
        float gating_threshold = 0.1f;      // Min graph activation to allow token
        bool adaptive_weighting = true;     // Learn α, β dynamically
        float weighting_lr = 0.001f;        // Learning rate for weights
        bool verbose = false;               // Logging
        
        Config();
    };
    
    explicit HybridPredictor(
        gnn::GNNPredictor* gnn,
        sequence::SequencePredictor* seq,
        tokenizer::Tokenizer* tok,
        Storage* storage,
        const Config& config = Config()
    );
    ~HybridPredictor();
    
    // ========================================================================
    // PREDICTION
    // ========================================================================
    
    /**
     * Predict next token with hybrid approach
     * Returns: (token_probabilities, graph_activations)
     */
    std::pair<std::vector<float>, std::vector<float>> predict_next(
        const std::vector<NodeID>& concept_context,     // From GNN
        const std::vector<uint32_t>& token_sequence     // From Transformer
    );
    
    /**
     * Generate text autoregressively
     * Uses both graph traversal and sequence prediction
     */
    std::vector<uint32_t> generate(
        const std::string& prompt,
        size_t max_length = 256,
        float temperature = 1.0f,
        bool graph_constrained = true
    );
    
    /**
     * Generate with explicit concept constraints
     * Forces output to include certain concepts from graph
     */
    std::vector<uint32_t> generate_with_concepts(
        const std::string& prompt,
        const std::vector<NodeID>& required_concepts,
        size_t max_length = 256
    );
    
    // ========================================================================
    // TRAINING
    // ========================================================================
    
    /**
     * Train hybrid system on paired data
     * Updates both sequence model and graph weights
     */
    float train_batch(
        const std::vector<std::string>& texts,
        const std::vector<std::vector<NodeID>>& graph_contexts
    );
    
    /**
     * Update weighting parameters (α, β)
     * Based on which source predicted better
     */
    void update_weights(float graph_accuracy, float sequence_accuracy);
    
    // ========================================================================
    // GRAPH-CONSTRAINED GENERATION
    // ========================================================================
    
    /**
     * Check if a token is valid according to graph
     * Returns activation strength (0 = invalid, 1 = strong)
     */
    float is_valid_token(uint32_t token_id, const std::vector<NodeID>& graph_context);
    
    /**
     * Filter token probabilities by graph validity
     * Zeros out tokens not supported by graph
     */
    std::vector<float> apply_graph_gate(
        const std::vector<float>& token_probs,
        const std::vector<NodeID>& graph_context
    );
    
    // ========================================================================
    // EXPLANATION
    // ========================================================================
    
    /**
     * Explain why a token was predicted
     * Returns: (graph_contribution, sequence_contribution, reasoning_path)
     */
    struct Explanation {
        float graph_contribution;
        float sequence_contribution;
        std::vector<NodeID> supporting_nodes;
        ReasoningPath reasoning_path;
        std::string text_explanation;
    };
    
    Explanation explain_prediction(
        uint32_t predicted_token,
        const std::vector<NodeID>& concept_context,
        const std::vector<uint32_t>& token_sequence
    );
    
    // ========================================================================
    // CONFIGURATION
    // ========================================================================
    
    void set_config(const Config& config);
    const Config& get_config() const;
    
    // ========================================================================
    // STATISTICS
    // ========================================================================
    
    struct Stats {
        uint64_t predictions_made = 0;
        float avg_graph_weight = 0.0f;        // Current α
        float avg_sequence_weight = 0.0f;     // Current β
        float graph_vetoes = 0;                // Times graph blocked output
        float avg_combined_confidence = 0.0f;
        uint64_t graph_dominated = 0;         // Predictions where graph > sequence
        uint64_t sequence_dominated = 0;      // Predictions where sequence > graph
    };
    
    Stats get_stats() const;
    void reset_stats();
    void print_stats() const;
    
private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace hybrid
} // namespace melvin

