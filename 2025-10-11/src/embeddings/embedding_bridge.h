#pragma once

#include "node_embeddings.h"
#include "token_embeddings.h"
#include "../../melvin_types.h"
#include "../../melvin_leap_nodes.h"
#include <vector>
#include <unordered_map>
#include <cmath>
#include <algorithm>

namespace melvin {
namespace embeddings {

// ==================== EMBEDDING BRIDGE CONFIGURATION ====================

struct EmbeddingBridgeConfig {
    int embedding_dim = 64;                     // Embedding dimension
    float lambda_graph_bias = 0.5f;             // Graph bias strength
    float learning_rate_embeddings = 0.01f;     // Embedding learning rate
    float similarity_threshold = 0.4f;          // Min similarity for bias
    float normalization_rate = 0.001f;          // Periodic normalization
    bool enable_embedding_bridge = true;        // Master switch
    bool use_leap_embeddings = true;            // Include leap nodes
    float leap_embedding_boost = 1.5f;          // Extra weight for leap concepts
};

// ==================== ACTIVATION VECTOR ====================

// Weighted combination of node embeddings representing current semantic context
struct ActivationVector {
    std::vector<float> embedding;               // The actual vector
    float total_activation;                     // Sum of activation weights
    std::vector<uint64_t> contributing_nodes;   // Which nodes contributed
    std::vector<float> node_weights;            // Their weights
    
    ActivationVector(int dim = 64) 
        : embedding(dim, 0.0f), total_activation(0.0f) {}
    
    bool is_zero() const {
        return total_activation < 1e-6f;
    }
    
    void normalize() {
        if (total_activation > 1e-6f) {
            for (float& val : embedding) {
                val /= total_activation;
            }
        }
    }
};

// ==================== TOKEN BIAS FIELD ====================

// Bias values for tokens based on graph/embedding similarity
struct TokenBiasField {
    std::unordered_map<std::string, float> biases;  // token -> bias value
    float max_bias;                                  // For normalization
    float min_bias;
    
    TokenBiasField() : max_bias(0.0f), min_bias(0.0f) {}
    
    float get_bias(const std::string& token) const {
        auto it = biases.find(token);
        return (it != biases.end()) ? it->second : 0.0f;
    }
    
    void normalize() {
        if (max_bias > min_bias + 1e-6f) {
            for (auto& [token, bias] : biases) {
                bias = (bias - min_bias) / (max_bias - min_bias);
            }
        }
    }
};

// ==================== EMBEDDING BRIDGE ====================

class EmbeddingBridge {
private:
    NodeEmbeddingManager node_embeddings_;
    TokenEmbeddingManager token_embeddings_;
    EmbeddingBridgeConfig config_;
    
    // Statistics
    uint64_t total_bias_computations_;
    uint64_t total_updates_;
    float avg_similarity_;
    
public:
    EmbeddingBridge(const EmbeddingBridgeConfig& config = EmbeddingBridgeConfig())
        : node_embeddings_(config.embedding_dim),
          token_embeddings_(config.embedding_dim),
          config_(config),
          total_bias_computations_(0),
          total_updates_(0),
          avg_similarity_(0.0f) {
    }
    
    // ==================== CORE OPERATIONS ====================
    
    // Compute activation vector from active nodes
    ActivationVector compute_activation_vector(
        const std::vector<uint64_t>& active_nodes,
        const std::vector<float>& activation_weights) {
        
        ActivationVector result(config_.embedding_dim);
        
        if (active_nodes.size() != activation_weights.size()) {
            return result;  // Size mismatch
        }
        
        for (size_t i = 0; i < active_nodes.size(); ++i) {
            uint64_t node_id = active_nodes[i];
            float weight = activation_weights[i];
            
            if (weight < 1e-6f) continue;
            
            const auto& node_emb = node_embeddings_.get_embedding_const(node_id);
            
            // Weighted sum
            for (int d = 0; d < config_.embedding_dim; ++d) {
                result.embedding[d] += weight * node_emb[d];
            }
            
            result.total_activation += weight;
            result.contributing_nodes.push_back(node_id);
            result.node_weights.push_back(weight);
        }
        
        result.normalize();
        return result;
    }
    
    // Include leap node embeddings in activation vector
    ActivationVector compute_activation_with_leaps(
        const std::vector<uint64_t>& active_nodes,
        const std::vector<float>& activation_weights,
        const std::vector<LeapNode>& active_leaps) {
        
        // Start with regular activation
        auto result = compute_activation_vector(active_nodes, activation_weights);
        
        if (!config_.use_leap_embeddings || active_leaps.empty()) {
            return result;
        }
        
        // Add leap contributions
        for (const auto& leap : active_leaps) {
            if (leap.concept_vector.empty()) continue;
            
            float leap_weight = leap.activation * leap.cohesion * config_.leap_embedding_boost;
            
            for (int d = 0; d < config_.embedding_dim && d < (int)leap.concept_vector.size(); ++d) {
                result.embedding[d] += leap_weight * leap.concept_vector[d];
            }
            
            result.total_activation += leap_weight;
        }
        
        result.normalize();
        return result;
    }
    
    // Compute token bias field from activation vector
    TokenBiasField compute_token_bias(
        const ActivationVector& activation,
        const std::vector<std::string>& candidate_tokens) {
        
        TokenBiasField bias_field;
        
        if (activation.is_zero()) {
            return bias_field;  // No active nodes
        }
        
        total_bias_computations_++;
        
        // Compute similarity for each candidate token
        for (const auto& token : candidate_tokens) {
            const auto& token_emb = token_embeddings_.get_embedding_const(token);
            
            float similarity = cosine_similarity(activation.embedding, token_emb);
            
            // Only add positive bias above threshold
            if (similarity > config_.similarity_threshold) {
                float bias = config_.lambda_graph_bias * similarity;
                bias_field.biases[token] = bias;
                
                bias_field.max_bias = std::max(bias_field.max_bias, bias);
                bias_field.min_bias = std::min(bias_field.min_bias, bias);
            }
        }
        
        return bias_field;
    }
    
    // Apply bias to log probabilities
    void apply_bias_to_logits(
        std::unordered_map<std::string, float>& logits,
        const TokenBiasField& bias_field) {
        
        for (auto& [token, logit] : logits) {
            float bias = bias_field.get_bias(token);
            logits[token] += bias;  // Add to log space
        }
    }
    
    // ==================== LEARNING & FEEDBACK ====================
    
    // Update embeddings after successful prediction
    void learn_from_prediction(
        const std::vector<uint64_t>& active_nodes,
        const std::vector<float>& activation_weights,
        const std::string& predicted_token,
        float reward) {
        
        if (reward < 1e-6f) return;
        
        total_updates_++;
        
        // Compute activation vector
        auto activation = compute_activation_vector(active_nodes, activation_weights);
        
        // Update token embedding toward activation
        const auto& token_emb = token_embeddings_.get_embedding_const(predicted_token);
        float similarity_before = cosine_similarity(activation.embedding, token_emb);
        
        token_embeddings_.update_toward(
            predicted_token,
            activation.embedding,
            config_.learning_rate_embeddings,
            reward
        );
        
        // Update contributing node embeddings toward token
        for (size_t i = 0; i < activation.contributing_nodes.size(); ++i) {
            uint64_t node_id = activation.contributing_nodes[i];
            float weight = activation.node_weights[i];
            
            if (weight > 0.1f) {  // Only significant contributors
                node_embeddings_.update_toward(
                    node_id,
                    token_embeddings_.get_embedding_const(predicted_token),
                    config_.learning_rate_embeddings * weight,
                    reward
                );
            }
        }
        
        // Update running similarity average
        const auto& token_emb_after = token_embeddings_.get_embedding_const(predicted_token);
        float similarity_after = cosine_similarity(activation.embedding, token_emb_after);
        avg_similarity_ = 0.95f * avg_similarity_ + 0.05f * similarity_after;
    }
    
    // Learn from misalignment (negative reward)
    void penalize_misalignment(
        const std::vector<uint64_t>& active_nodes,
        const std::vector<float>& activation_weights,
        const std::string& predicted_token) {
        
        learn_from_prediction(active_nodes, activation_weights, predicted_token, -0.3f);
    }
    
    // ==================== UTILITIES ====================
    
    // Cosine similarity between two embeddings
    float cosine_similarity(const std::vector<float>& a, const std::vector<float>& b) const {
        if (a.size() != b.size() || a.empty()) return 0.0f;
        
        float dot = 0.0f;
        float norm_a = 0.0f;
        float norm_b = 0.0f;
        
        for (size_t i = 0; i < a.size(); ++i) {
            dot += a[i] * b[i];
            norm_a += a[i] * a[i];
            norm_b += b[i] * b[i];
        }
        
        if (norm_a < 1e-9f || norm_b < 1e-9f) return 0.0f;
        
        return dot / (std::sqrt(norm_a) * std::sqrt(norm_b));
    }
    
    // Initialize node embedding from text
    void initialize_node_from_text(uint64_t node_id, const std::string& text) {
        if (node_embeddings_.has_embedding(node_id)) return;
        
        // Get token embedding for the text
        const auto& token_emb = token_embeddings_.get_embedding_const(text);
        
        // Initialize node with token embedding
        auto& node_emb = node_embeddings_.get_embedding(node_id);
        node_emb = token_emb;
    }
    
    // Batch initialize from node list
    void batch_initialize_nodes(
        const std::vector<uint64_t>& node_ids,
        const std::vector<std::string>& node_texts) {
        
        if (node_ids.size() != node_texts.size()) return;
        
        for (size_t i = 0; i < node_ids.size(); ++i) {
            initialize_node_from_text(node_ids[i], node_texts[i]);
        }
    }
    
    // ==================== STATISTICS ====================
    
    void print_statistics() const {
        std::cout << "\n📊 Embedding Bridge Statistics:\n";
        std::cout << "  Node embeddings: " << node_embeddings_.size() << "\n";
        std::cout << "  Token embeddings: " << token_embeddings_.size() << "\n";
        std::cout << "  Embedding dimension: " << config_.embedding_dim << "\n";
        std::cout << "  Total bias computations: " << total_bias_computations_ << "\n";
        std::cout << "  Total updates: " << total_updates_ << "\n";
        std::cout << "  Average similarity: " << std::fixed << std::setprecision(3) 
                  << avg_similarity_ << "\n";
        std::cout << "  Lambda (bias strength): " << config_.lambda_graph_bias << "\n";
    }
    
    // ==================== PERSISTENCE ====================
    
    void save() {
        node_embeddings_.save("data/node_embeddings.bin");
        token_embeddings_.save("data/token_embeddings.bin");
    }
    
    void load() {
        node_embeddings_.load("data/node_embeddings.bin");
        token_embeddings_.load("data/token_embeddings.bin");
    }
    
    // ==================== CONFIGURATION ====================
    
    void set_config(const EmbeddingBridgeConfig& config) {
        config_ = config;
    }
    
    const EmbeddingBridgeConfig& get_config() const {
        return config_;
    }
    
    // Access to managers (for advanced use)
    NodeEmbeddingManager& node_manager() { return node_embeddings_; }
    TokenEmbeddingManager& token_manager() { return token_embeddings_; }
};

// ==================== INTEGRATION HELPERS ====================

// Helper to extract tokens from node text
inline std::string node_to_token(const std::string& node_text) {
    std::string token = node_text;
    std::transform(token.begin(), token.end(), token.begin(), ::tolower);
    
    // Remove punctuation
    token.erase(std::remove_if(token.begin(), token.end(),
                [](char c) { return !std::isalnum(c) && c != '\'' && c != '-'; }),
                token.end());
    
    return token;
}

// Compute activation weights from node activations and leap contributions
inline std::pair<std::vector<uint64_t>, std::vector<float>> 
compute_combined_activations(
    const std::vector<uint64_t>& graph_nodes,
    const std::vector<float>& graph_activations,
    const std::vector<LeapNode>& active_leaps) {
    
    std::vector<uint64_t> all_nodes = graph_nodes;
    std::vector<float> all_weights = graph_activations;
    
    // Add leap node members with boosted weights
    for (const auto& leap : active_leaps) {
        float leap_weight = leap.activation * leap.cohesion;
        
        for (uint64_t member : leap.members) {
            all_nodes.push_back(member);
            all_weights.push_back(leap_weight);
        }
    }
    
    return {all_nodes, all_weights};
}

} // namespace embeddings
} // namespace melvin

