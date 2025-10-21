#include "hybrid_predictor.h"
#include <algorithm>
#include <iostream>
#include <numeric>
#include <cmath>

namespace melvin {
namespace hybrid {

// ============================================================================
// Config Implementation
// ============================================================================

HybridPredictor::Config::Config() {}

// ============================================================================
// Implementation Class
// ============================================================================

class HybridPredictor::Impl {
public:
    Config config_;
    gnn::GNNPredictor* gnn_;
    sequence::SequencePredictor* seq_;
    tokenizer::Tokenizer* tok_;
    Storage* storage_;
    
    // Adaptive weights
    float current_graph_weight_;
    float current_sequence_weight_;
    
    // Statistics
    Stats stats_;
    
    Impl(gnn::GNNPredictor* gnn, sequence::SequencePredictor* seq, 
         tokenizer::Tokenizer* tok, Storage* storage, const Config& config)
        : config_(config), gnn_(gnn), seq_(seq), tok_(tok), storage_(storage)
        , current_graph_weight_(config.graph_weight)
        , current_sequence_weight_(config.sequence_weight) {}
    
    std::pair<std::vector<float>, std::vector<float>> predict_next(
        const std::vector<NodeID>& concept_context,
        const std::vector<uint32_t>& token_sequence
    ) {
        // Get predictions from both systems
        auto graph_probs = predict_from_graph(concept_context);
        auto sequence_probs = seq_->predict_next(token_sequence);
        
        // Ensure same size
        size_t vocab_size = std::max(graph_probs.size(), sequence_probs.size());
        graph_probs.resize(vocab_size, 0.0f);
        sequence_probs.resize(vocab_size, 0.0f);
        
        // Combine: P = α * P_graph + β * P_seq
        std::vector<float> combined(vocab_size);
        float alpha = current_graph_weight_;
        float beta = current_sequence_weight_;
        
        for (size_t i = 0; i < vocab_size; i++) {
            combined[i] = alpha * graph_probs[i] + beta * sequence_probs[i];
        }
        
        // Apply graph gating if enabled
        if (config_.graph_gates_output) {
            combined = apply_graph_gate(combined, concept_context);
        }
        
        // Renormalize
        float sum = std::accumulate(combined.begin(), combined.end(), 0.0f);
        if (sum > 0.0f) {
            for (auto& p : combined) {
                p /= sum;
            }
        }
        
        // Update statistics
        stats_.predictions_made++;
        stats_.avg_graph_weight = stats_.avg_graph_weight * 0.99f + alpha * 0.01f;
        stats_.avg_sequence_weight = stats_.avg_sequence_weight * 0.99f + beta * 0.01f;
        
        // Determine which dominated
        float graph_strength = 0.0f, seq_strength = 0.0f;
        for (size_t i = 0; i < vocab_size; i++) {
            graph_strength += graph_probs[i] * combined[i];
            seq_strength += sequence_probs[i] * combined[i];
        }
        
        if (graph_strength > seq_strength) {
            stats_.graph_dominated++;
        } else {
            stats_.sequence_dominated++;
        }
        
        return {combined, graph_probs};
    }
    
    std::vector<float> predict_from_graph(const std::vector<NodeID>& concept_context) {
        if (concept_context.empty() || !gnn_ || !tok_) {
            return std::vector<float>(tok_->vocab_size(), 0.0f);
        }
        
        // Get nodes from storage
        std::vector<Node> nodes;
        for (NodeID id : concept_context) {
            Node node;
            if (storage_ && storage_->get_node(id, node)) {
                nodes.push_back(node);
            }
        }
        
        if (nodes.empty()) {
            return std::vector<float>(tok_->vocab_size(), 0.0f);
        }
        
        // Get predicted activations from GNN
        auto predicted_activations = gnn_->predict_activations(nodes);
        
        // Convert node activations to token probabilities
        std::vector<float> token_probs(tok_->vocab_size(), 0.0f);
        
        for (size_t i = 0; i < nodes.size() && i < predicted_activations.size(); i++) {
            NodeID node_id = nodes[i].id;
            float activation = predicted_activations[i];
            
            // Map node to token
            if (tok_->has_node_mapping(node_id)) {
                uint32_t token_id = tok_->node_to_token(node_id);
                if (token_id < token_probs.size()) {
                    token_probs[token_id] += activation;
                }
            }
        }
        
        // Normalize to probability distribution
        float sum = std::accumulate(token_probs.begin(), token_probs.end(), 0.0f);
        if (sum > 0.0f) {
            for (auto& p : token_probs) {
                p /= sum;
            }
        } else {
            // Uniform distribution if no graph support
            float uniform = 1.0f / token_probs.size();
            std::fill(token_probs.begin(), token_probs.end(), uniform);
        }
        
        return token_probs;
    }
    
    std::vector<uint32_t> generate(
        const std::string& prompt,
        size_t max_length,
        float temperature,
        bool graph_constrained
    ) {
        // Encode prompt
        auto tokens = tok_->encode(prompt);
        
        // Get initial concept context
        std::vector<NodeID> concept_context;
        for (uint32_t token_id : tokens) {
            if (tok_->has_token_mapping(token_id)) {
                concept_context.push_back(tok_->token_to_node(token_id));
            }
        }
        
        // Generate autoregressively
        for (size_t i = tokens.size(); i < max_length; i++) {
            auto [probs, graph_acts] = predict_next(concept_context, tokens);
            
            // Apply temperature
            if (temperature != 1.0f) {
                for (auto& p : probs) {
                    p = std::pow(p, 1.0f / temperature);
                }
                float sum = std::accumulate(probs.begin(), probs.end(), 0.0f);
                for (auto& p : probs) {
                    p /= sum;
                }
            }
            
            // Sample
            uint32_t next_token = sample_token(probs);
            
            // Check for EOS
            if (next_token == tok_->eos_token_id()) break;
            
            tokens.push_back(next_token);
            
            // Update concept context
            if (tok_->has_token_mapping(next_token)) {
                concept_context.push_back(tok_->token_to_node(next_token));
            }
        }
        
        return tokens;
    }
    
    std::vector<uint32_t> generate_with_concepts(
        const std::string& prompt,
        const std::vector<NodeID>& required_concepts,
        size_t max_length
    ) {
        auto tokens = tok_->encode(prompt);
        std::vector<NodeID> concept_context = required_concepts;
        
        // Add concepts from prompt
        for (uint32_t token_id : tokens) {
            if (tok_->has_token_mapping(token_id)) {
                concept_context.push_back(tok_->token_to_node(token_id));
            }
        }
        
        for (size_t i = tokens.size(); i < max_length; i++) {
            auto [probs, _] = predict_next(concept_context, tokens);
            
            uint32_t next_token = sample_token(probs);
            if (next_token == tok_->eos_token_id()) break;
            
            tokens.push_back(next_token);
            
            if (tok_->has_token_mapping(next_token)) {
                concept_context.push_back(tok_->token_to_node(next_token));
            }
        }
        
        return tokens;
    }
    
    uint32_t sample_token(const std::vector<float>& probs) {
        float r = static_cast<float>(rand()) / RAND_MAX;
        float cumsum = 0.0f;
        
        for (size_t i = 0; i < probs.size(); i++) {
            cumsum += probs[i];
            if (r <= cumsum) {
                return i;
            }
        }
        
        return probs.size() > 0 ? probs.size() - 1 : 0;
    }
    
    float train_batch(
        const std::vector<std::string>& texts,
        const std::vector<std::vector<NodeID>>& graph_contexts
    ) {
        float total_loss = 0.0f;
        
        for (size_t i = 0; i < texts.size(); i++) {
            const auto& text = texts[i];
            const auto& context = i < graph_contexts.size() ? 
                                 graph_contexts[i] : std::vector<NodeID>();
            
            // Encode text
            auto tokens = tok_->encode(text);
            if (tokens.size() < 2) continue;
            
            // Prepare sequences and targets
            std::vector<std::vector<uint32_t>> sequences;
            std::vector<std::vector<uint32_t>> targets;
            
            for (size_t j = 0; j < tokens.size() - 1; j++) {
                sequences.push_back(std::vector<uint32_t>(tokens.begin(), tokens.begin() + j + 1));
                targets.push_back({tokens[j + 1]});
            }
            
            // Train sequence model
            if (seq_) {
                float loss = seq_->train_batch(sequences, targets);
                total_loss += loss;
            }
            
            // Train GNN (if implemented)
            // This would update node embeddings based on co-occurrence
        }
        
        return total_loss / std::max(texts.size(), size_t(1));
    }
    
    void update_weights(float graph_accuracy, float sequence_accuracy) {
        if (!config_.adaptive_weighting) return;
        
        // Update weights based on relative accuracy
        float total = graph_accuracy + sequence_accuracy;
        if (total > 0.0f) {
            float target_graph = graph_accuracy / total;
            float target_sequence = sequence_accuracy / total;
            
            // Gradient step toward target
            current_graph_weight_ += config_.weighting_lr * (target_graph - current_graph_weight_);
            current_sequence_weight_ += config_.weighting_lr * (target_sequence - current_sequence_weight_);
            
            // Ensure they sum to 1.0
            float sum = current_graph_weight_ + current_sequence_weight_;
            current_graph_weight_ /= sum;
            current_sequence_weight_ /= sum;
        }
    }
    
    float is_valid_token(uint32_t token_id, const std::vector<NodeID>& graph_context) {
        if (!tok_->has_token_mapping(token_id)) {
            return 0.5f;  // Unknown tokens get medium validity
        }
        
        NodeID token_node = tok_->token_to_node(token_id);
        
        // Check if token's concept is in graph context or neighbors
        for (NodeID context_node : graph_context) {
            if (token_node == context_node) {
                return 1.0f;  // Exact match
            }
            
            // Check if connected in graph
            if (storage_) {
                auto edges = storage_->get_edges(context_node, token_node);
                if (!edges.empty()) {
                    // Return edge weight as validity
                    return edges[0].get_weight();
                }
            }
        }
        
        return 0.1f;  // Low validity but not zero (allow exploration)
    }
    
    std::vector<float> apply_graph_gate(
        const std::vector<float>& token_probs,
        const std::vector<NodeID>& graph_context
    ) {
        std::vector<float> gated = token_probs;
        
        for (size_t i = 0; i < gated.size(); i++) {
            float validity = is_valid_token(i, graph_context);
            
            if (validity < config_.gating_threshold) {
                gated[i] *= validity / config_.gating_threshold;  // Reduce but don't zero
                stats_.graph_vetoes += 1.0f;
            }
        }
        
        return gated;
    }
    
    HybridPredictor::Explanation explain_prediction(
        uint32_t predicted_token,
        const std::vector<NodeID>& concept_context,
        const std::vector<uint32_t>& token_sequence
    ) {
        Explanation exp;
        
        // Get predictions from both sources
        auto graph_probs = predict_from_graph(concept_context);
        auto seq_probs = seq_->predict_next(token_sequence);
        
        if (predicted_token < graph_probs.size() && predicted_token < seq_probs.size()) {
            exp.graph_contribution = graph_probs[predicted_token] * current_graph_weight_;
            exp.sequence_contribution = seq_probs[predicted_token] * current_sequence_weight_;
        }
        
        // Find supporting nodes
        if (tok_->has_token_mapping(predicted_token)) {
            NodeID predicted_node = tok_->token_to_node(predicted_token);
            
            for (NodeID context_node : concept_context) {
                if (storage_) {
                    auto edges = storage_->get_edges(context_node, predicted_node);
                    if (!edges.empty()) {
                        exp.supporting_nodes.push_back(context_node);
                    }
                }
            }
        }
        
        // Build text explanation
        std::string token_str = tok_->get_token(predicted_token);
        exp.text_explanation = "Token '" + token_str + "' predicted with:\n";
        exp.text_explanation += "  Graph: " + std::to_string(exp.graph_contribution) + "\n";
        exp.text_explanation += "  Sequence: " + std::to_string(exp.sequence_contribution) + "\n";
        exp.text_explanation += "  Supporting nodes: " + std::to_string(exp.supporting_nodes.size());
        
        return exp;
    }
};

// ============================================================================
// Public Interface
// ============================================================================

HybridPredictor::HybridPredictor(
    gnn::GNNPredictor* gnn,
    sequence::SequencePredictor* seq,
    tokenizer::Tokenizer* tok,
    Storage* storage,
    const Config& config
) : impl_(std::make_unique<Impl>(gnn, seq, tok, storage, config)) {}

HybridPredictor::~HybridPredictor() = default;

std::pair<std::vector<float>, std::vector<float>> HybridPredictor::predict_next(
    const std::vector<NodeID>& concept_context,
    const std::vector<uint32_t>& token_sequence
) {
    return impl_->predict_next(concept_context, token_sequence);
}

std::vector<uint32_t> HybridPredictor::generate(
    const std::string& prompt,
    size_t max_length,
    float temperature,
    bool graph_constrained
) {
    return impl_->generate(prompt, max_length, temperature, graph_constrained);
}

std::vector<uint32_t> HybridPredictor::generate_with_concepts(
    const std::string& prompt,
    const std::vector<NodeID>& required_concepts,
    size_t max_length
) {
    return impl_->generate_with_concepts(prompt, required_concepts, max_length);
}

float HybridPredictor::train_batch(
    const std::vector<std::string>& texts,
    const std::vector<std::vector<NodeID>>& graph_contexts
) {
    return impl_->train_batch(texts, graph_contexts);
}

void HybridPredictor::update_weights(float graph_accuracy, float sequence_accuracy) {
    impl_->update_weights(graph_accuracy, sequence_accuracy);
}

float HybridPredictor::is_valid_token(uint32_t token_id, const std::vector<NodeID>& graph_context) {
    return impl_->is_valid_token(token_id, graph_context);
}

std::vector<float> HybridPredictor::apply_graph_gate(
    const std::vector<float>& token_probs,
    const std::vector<NodeID>& graph_context
) {
    return impl_->apply_graph_gate(token_probs, graph_context);
}

HybridPredictor::Explanation HybridPredictor::explain_prediction(
    uint32_t predicted_token,
    const std::vector<NodeID>& concept_context,
    const std::vector<uint32_t>& token_sequence
) {
    return impl_->explain_prediction(predicted_token, concept_context, token_sequence);
}

void HybridPredictor::set_config(const Config& config) {
    impl_->config_ = config;
}

const HybridPredictor::Config& HybridPredictor::get_config() const {
    return impl_->config_;
}

HybridPredictor::Stats HybridPredictor::get_stats() const {
    return impl_->stats_;
}

void HybridPredictor::reset_stats() {
    impl_->stats_ = Stats();
}

void HybridPredictor::print_stats() const {
    auto stats = get_stats();
    std::cout << "\n=== Hybrid Predictor Statistics ===\n";
    std::cout << "Predictions made: " << stats.predictions_made << "\n";
    std::cout << "Average graph weight: " << stats.avg_graph_weight << "\n";
    std::cout << "Average sequence weight: " << stats.avg_sequence_weight << "\n";
    std::cout << "Graph vetoes: " << stats.graph_vetoes << "\n";
    std::cout << "Graph dominated: " << stats.graph_dominated << "\n";
    std::cout << "Sequence dominated: " << stats.sequence_dominated << "\n";
    std::cout << "===================================\n\n";
}

} // namespace hybrid
} // namespace melvin

