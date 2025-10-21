#include "sequence_predictor.h"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <iostream>
#include <numeric>
#include <random>

namespace melvin {
namespace sequence {

// ============================================================================
// Helper Functions
// ============================================================================

static std::vector<float> softmax(const std::vector<float>& logits) {
    std::vector<float> probs(logits.size());
    
    // Find max for numerical stability
    float max_logit = *std::max_element(logits.begin(), logits.end());
    
    // Compute exp and sum
    float sum = 0.0f;
    for (size_t i = 0; i < logits.size(); i++) {
        probs[i] = std::exp(logits[i] - max_logit);
        sum += probs[i];
    }
    
    // Normalize
    for (auto& p : probs) {
        p /= sum;
    }
    
    return probs;
}

static float cross_entropy_loss(const std::vector<float>& probs, uint32_t target) {
    if (target >= probs.size()) return 1000.0f;  // Large loss for invalid
    return -std::log(std::max(probs[target], 1e-10f));
}

static uint32_t sample_from_distribution(const std::vector<float>& probs, float temperature = 1.0f) {
    // Apply temperature
    std::vector<float> adjusted = probs;
    if (temperature != 1.0f) {
        for (auto& p : adjusted) {
            p = std::pow(p, 1.0f / temperature);
        }
        // Renormalize
        float sum = std::accumulate(adjusted.begin(), adjusted.end(), 0.0f);
        for (auto& p : adjusted) {
            p /= sum;
        }
    }
    
    // Sample
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::discrete_distribution<> dist(adjusted.begin(), adjusted.end());
    return dist(gen);
}

// ============================================================================
// Config Implementation
// ============================================================================

SequencePredictor::Config::Config() {}

// ============================================================================
// Implementation Class
// ============================================================================

class SequencePredictor::Impl {
public:
    Config config_;
    Stats stats_;
    
    // Simplified weight matrices (in a full implementation, these would be proper transformers)
    // Token embeddings: vocab_size × d_model
    std::vector<std::vector<float>> token_embeddings_;
    
    // Positional embeddings: context_len × d_model
    std::vector<std::vector<float>> positional_embeddings_;
    
    // Output projection: d_model × vocab_size
    std::vector<std::vector<float>> output_projection_;
    
    // Graph bias projection: d_model × vocab_size
    std::vector<std::vector<float>> graph_bias_projection_;
    
    Impl(const Config& config) : config_(config) {
        initialize_weights();
    }
    
    void initialize_weights() {
        // Initialize token embeddings
        token_embeddings_.resize(config_.vocab_size);
        for (auto& embedding : token_embeddings_) {
            embedding.resize(config_.d_model);
            for (auto& val : embedding) {
                val = random_normal(0.0f, 0.02f);
            }
        }
        
        // Initialize positional embeddings
        positional_embeddings_.resize(config_.context_len);
        for (size_t pos = 0; pos < config_.context_len; pos++) {
            positional_embeddings_[pos].resize(config_.d_model);
            for (int i = 0; i < config_.d_model; i++) {
                // Sinusoidal positional encoding
                float angle = pos / std::pow(10000.0f, (2.0f * i) / config_.d_model);
                positional_embeddings_[pos][i] = (i % 2 == 0) ? std::sin(angle) : std::cos(angle);
            }
        }
        
        // Initialize output projection
        output_projection_.resize(config_.d_model);
        for (auto& row : output_projection_) {
            row.resize(config_.vocab_size);
            for (auto& val : row) {
                val = random_normal(0.0f, 0.02f);
            }
        }
        
        // Initialize graph bias projection
        if (config_.use_graph_bias) {
            graph_bias_projection_.resize(config_.d_model);
            for (auto& row : graph_bias_projection_) {
                row.resize(config_.vocab_size);
                for (auto& val : row) {
                    val = random_normal(0.0f, 0.02f);
                }
            }
        }
    }
    
    float random_normal(float mean, float stddev) {
        static std::random_device rd;
        static std::mt19937 gen(rd());
        std::normal_distribution<> dist(mean, stddev);
        return dist(gen);
    }
    
    std::vector<float> embed_sequence(const std::vector<uint32_t>& tokens) const {
        std::vector<float> embedding(config_.d_model, 0.0f);
        
        for (size_t i = 0; i < tokens.size(); i++) {
            uint32_t token_id = tokens[i];
            if (token_id >= config_.vocab_size) continue;
            
            size_t pos = std::min(i, (size_t)(config_.context_len - 1));
            
            // Add token embedding + positional embedding
            for (int j = 0; j < config_.d_model; j++) {
                embedding[j] += token_embeddings_[token_id][j] + positional_embeddings_[pos][j];
            }
        }
        
        // Average pooling
        if (!tokens.empty()) {
            for (auto& val : embedding) {
                val /= tokens.size();
            }
        }
        
        return embedding;
    }
    
    std::vector<float> predict_next(const std::vector<uint32_t>& token_sequence) const {
        // Embed the sequence
        auto embedding = embed_sequence(token_sequence);
        
        // Project to vocabulary logits
        std::vector<float> logits(config_.vocab_size, 0.0f);
        for (int i = 0; i < config_.d_model; i++) {
            for (size_t j = 0; j < config_.vocab_size; j++) {
                logits[j] += embedding[i] * output_projection_[i][j];
            }
        }
        
        return softmax(logits);
    }
    
    std::vector<float> predict_next_with_graph(
        const std::vector<uint32_t>& token_sequence,
        const std::vector<NodeID>& graph_context,
        const std::vector<float>& graph_activations
    ) const {
        // Get base prediction
        auto probs = predict_next(token_sequence);
        
        if (!config_.use_graph_bias || graph_context.empty()) {
            return probs;
        }
        
        // Compute graph bias
        std::vector<float> graph_bias(config_.vocab_size, 0.0f);
        
        for (size_t i = 0; i < graph_context.size(); i++) {
            NodeID node_id = graph_context[i];
            float activation = i < graph_activations.size() ? graph_activations[i] : 1.0f;
            
            // Use node_id to bias toward related tokens
            // (In full implementation, this would use learned embeddings)
            if (node_id < config_.vocab_size) {
                graph_bias[node_id] += activation;
            }
        }
        
        // Normalize graph bias
        float bias_sum = std::accumulate(graph_bias.begin(), graph_bias.end(), 0.0f);
        if (bias_sum > 0.0f) {
            for (auto& b : graph_bias) {
                b /= bias_sum;
            }
        }
        
        // Combine: P_final = (1 - α) * P_seq + α * P_graph
        std::vector<float> combined(config_.vocab_size);
        float alpha = config_.graph_bias_weight;
        for (size_t i = 0; i < config_.vocab_size; i++) {
            combined[i] = (1.0f - alpha) * probs[i] + alpha * graph_bias[i];
        }
        
        return combined;
    }
    
    std::vector<uint32_t> generate(
        const std::vector<uint32_t>& prefix,
        size_t max_length,
        float temperature,
        float top_p
    ) const {
        std::vector<uint32_t> generated = prefix;
        
        for (size_t i = prefix.size(); i < max_length; i++) {
            auto probs = predict_next(generated);
            
            // Nucleus sampling (top-p)
            if (top_p < 1.0f) {
                // Sort by probability
                std::vector<std::pair<float, uint32_t>> sorted_probs;
                for (size_t j = 0; j < probs.size(); j++) {
                    sorted_probs.push_back({probs[j], j});
                }
                std::sort(sorted_probs.begin(), sorted_probs.end(), std::greater<>());
                
                // Find cutoff
                float cumsum = 0.0f;
                size_t cutoff = 0;
                for (size_t j = 0; j < sorted_probs.size(); j++) {
                    cumsum += sorted_probs[j].first;
                    cutoff = j;
                    if (cumsum >= top_p) break;
                }
                
                // Zero out below cutoff
                std::vector<float> filtered(probs.size(), 0.0f);
                for (size_t j = 0; j <= cutoff; j++) {
                    filtered[sorted_probs[j].second] = sorted_probs[j].first;
                }
                
                // Renormalize
                float sum = std::accumulate(filtered.begin(), filtered.end(), 0.0f);
                if (sum > 0.0f) {
                    for (auto& p : filtered) p /= sum;
                    probs = filtered;
                }
            }
            
            // Sample
            uint32_t next_token = sample_from_distribution(probs, temperature);
            generated.push_back(next_token);
            
            // Check for EOS (token_id = 2)
            if (next_token == 2) break;
        }
        
        return generated;
    }
    
    float train_batch(
        const std::vector<std::vector<uint32_t>>& sequences,
        const std::vector<std::vector<uint32_t>>& targets
    ) {
        float total_loss = 0.0f;
        size_t total_tokens = 0;
        
        for (size_t i = 0; i < sequences.size(); i++) {
            const auto& seq = sequences[i];
            const auto& tgt = targets[i];
            
            for (size_t j = 0; j < std::min(seq.size(), tgt.size()); j++) {
                // Forward pass
                std::vector<uint32_t> prefix(seq.begin(), seq.begin() + j + 1);
                auto probs = predict_next(prefix);
                
                // Compute loss
                if (j < tgt.size()) {
                    float loss = cross_entropy_loss(probs, tgt[j]);
                    total_loss += loss;
                    total_tokens++;
                    
                    // Backward pass (simplified - just update stats)
                    // In a full implementation, this would do proper backpropagation
                }
            }
        }
        
        float avg_loss = total_tokens > 0 ? total_loss / total_tokens : 0.0f;
        stats_.training_steps++;
        stats_.avg_loss = stats_.avg_loss * 0.99f + avg_loss * 0.01f;
        stats_.tokens_processed += total_tokens;
        
        return avg_loss;
    }
    
    float train_batch_with_graph(
        const std::vector<std::vector<uint32_t>>& sequences,
        const std::vector<std::vector<uint32_t>>& targets,
        const std::vector<std::vector<NodeID>>& graph_contexts,
        const std::vector<std::vector<float>>& graph_activations
    ) {
        float total_loss = 0.0f;
        size_t total_tokens = 0;
        float graph_contribution = 0.0f;
        
        for (size_t i = 0; i < sequences.size(); i++) {
            const auto& seq = sequences[i];
            const auto& tgt = targets[i];
            const auto& ctx = i < graph_contexts.size() ? graph_contexts[i] : std::vector<NodeID>();
            const auto& act = i < graph_activations.size() ? graph_activations[i] : std::vector<float>();
            
            for (size_t j = 0; j < std::min(seq.size(), tgt.size()); j++) {
                std::vector<uint32_t> prefix(seq.begin(), seq.begin() + j + 1);
                
                // Predict with and without graph
                auto probs_with_graph = predict_next_with_graph(prefix, ctx, act);
                auto probs_without_graph = predict_next(prefix);
                
                // Compute loss
                if (j < tgt.size()) {
                    float loss_with = cross_entropy_loss(probs_with_graph, tgt[j]);
                    float loss_without = cross_entropy_loss(probs_without_graph, tgt[j]);
                    
                    total_loss += loss_with;
                    total_tokens++;
                    
                    // Track how much graph helps
                    if (loss_without > loss_with) {
                        graph_contribution += (loss_without - loss_with);
                    }
                }
            }
        }
        
        float avg_loss = total_tokens > 0 ? total_loss / total_tokens : 0.0f;
        stats_.training_steps++;
        stats_.avg_loss = stats_.avg_loss * 0.99f + avg_loss * 0.01f;
        stats_.tokens_processed += total_tokens;
        stats_.graph_bias_contribution = total_tokens > 0 ? graph_contribution / total_tokens : 0.0f;
        
        return avg_loss;
    }
    
    float compute_perplexity(const std::vector<std::vector<uint32_t>>& sequences) const {
        float total_loss = 0.0f;
        size_t total_tokens = 0;
        
        for (const auto& seq : sequences) {
            for (size_t i = 0; i < seq.size() - 1; i++) {
                std::vector<uint32_t> prefix(seq.begin(), seq.begin() + i + 1);
                auto probs = predict_next(prefix);
                
                uint32_t target = seq[i + 1];
                float loss = cross_entropy_loss(probs, target);
                total_loss += loss;
                total_tokens++;
            }
        }
        
        float avg_loss = total_tokens > 0 ? total_loss / total_tokens : 0.0f;
        return std::exp(avg_loss);
    }
    
    bool save(const std::string& path) const {
        std::ofstream file(path, std::ios::binary);
        if (!file) return false;
        
        // Write config
        file.write(reinterpret_cast<const char*>(&config_), sizeof(config_));
        
        // Write token embeddings
        for (const auto& embedding : token_embeddings_) {
            file.write(reinterpret_cast<const char*>(embedding.data()), 
                      embedding.size() * sizeof(float));
        }
        
        // Write output projection
        for (const auto& row : output_projection_) {
            file.write(reinterpret_cast<const char*>(row.data()), 
                      row.size() * sizeof(float));
        }
        
        return true;
    }
    
    bool load(const std::string& path) {
        std::ifstream file(path, std::ios::binary);
        if (!file) return false;
        
        // Read config
        file.read(reinterpret_cast<char*>(&config_), sizeof(config_));
        
        // Reinitialize with loaded config
        initialize_weights();
        
        // Read token embeddings
        for (auto& embedding : token_embeddings_) {
            file.read(reinterpret_cast<char*>(embedding.data()), 
                     embedding.size() * sizeof(float));
        }
        
        // Read output projection
        for (auto& row : output_projection_) {
            file.read(reinterpret_cast<char*>(row.data()), 
                     row.size() * sizeof(float));
        }
        
        return true;
    }
};

// ============================================================================
// Public Interface
// ============================================================================

SequencePredictor::SequencePredictor(const Config& config)
    : impl_(std::make_unique<Impl>(config)) {}

SequencePredictor::~SequencePredictor() = default;

std::vector<float> SequencePredictor::predict_next(const std::vector<uint32_t>& token_sequence) const {
    return impl_->predict_next(token_sequence);
}

std::vector<float> SequencePredictor::predict_next_with_graph(
    const std::vector<uint32_t>& token_sequence,
    const std::vector<NodeID>& graph_context,
    const std::vector<float>& graph_activations
) const {
    return impl_->predict_next_with_graph(token_sequence, graph_context, graph_activations);
}

std::vector<uint32_t> SequencePredictor::generate(
    const std::vector<uint32_t>& prefix,
    size_t max_length,
    float temperature,
    float top_p
) const {
    return impl_->generate(prefix, max_length, temperature, top_p);
}

float SequencePredictor::train_batch(
    const std::vector<std::vector<uint32_t>>& sequences,
    const std::vector<std::vector<uint32_t>>& targets
) {
    return impl_->train_batch(sequences, targets);
}

float SequencePredictor::train_batch_with_graph(
    const std::vector<std::vector<uint32_t>>& sequences,
    const std::vector<std::vector<uint32_t>>& targets,
    const std::vector<std::vector<NodeID>>& graph_contexts,
    const std::vector<std::vector<float>>& graph_activations
) {
    return impl_->train_batch_with_graph(sequences, targets, graph_contexts, graph_activations);
}

float SequencePredictor::compute_perplexity(
    const std::vector<std::vector<uint32_t>>& sequences
) const {
    return impl_->compute_perplexity(sequences);
}

void SequencePredictor::set_config(const Config& config) {
    impl_->config_ = config;
}

const SequencePredictor::Config& SequencePredictor::get_config() const {
    return impl_->config_;
}

bool SequencePredictor::save(const std::string& path) const {
    return impl_->save(path);
}

bool SequencePredictor::load(const std::string& path) {
    return impl_->load(path);
}

SequencePredictor::Stats SequencePredictor::get_stats() const {
    auto stats = impl_->stats_;
    stats.perplexity = std::exp(stats.avg_loss);
    return stats;
}

void SequencePredictor::reset_stats() {
    impl_->stats_ = Stats();
}

void SequencePredictor::print_stats() const {
    auto stats = get_stats();
    std::cout << "\n=== Sequence Predictor Statistics ===\n";
    std::cout << "Training steps: " << stats.training_steps << "\n";
    std::cout << "Average loss: " << stats.avg_loss << "\n";
    std::cout << "Perplexity: " << stats.perplexity << "\n";
    std::cout << "Graph bias contribution: " << stats.graph_bias_contribution << "\n";
    std::cout << "Tokens processed: " << stats.tokens_processed << "\n";
    std::cout << "=====================================\n\n";
}

} // namespace sequence
} // namespace melvin

