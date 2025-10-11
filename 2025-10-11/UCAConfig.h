#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <deque>
#include <chrono>
#include <algorithm>
#include <cmath>

namespace melvin {

// Forward declarations
using NodeID = std::array<uint8_t, 32>;
using EdgeID = std::array<uint8_t, 32>;

/**
 * Enhanced UCA Configuration for LLM-Style Reasoning
 * 
 * This configuration enables Melvin to perform soft attention-style reasoning,
 * probabilistic output generation, and embedding-aware processing.
 */
struct UCAConfig {
    // Core LLM-style features
    bool use_soft_traversal = true;
    bool use_probabilistic_output = true;
    bool use_embeddings = true;
    bool dual_state_evolution = true;
    bool use_context_buffer = true;
    bool use_feedback_loop = true;
    
    // Soft traversal parameters
    float attention_dropout = 0.1f;
    int max_attention_depth = 8;
    float attention_temperature = 1.0f;
    
    // Probabilistic output parameters
    float output_temperature = 0.8f;
    float output_top_p = 0.9f;
    int max_output_length = 50;
    float repetition_penalty = 1.1f;
    
    // Embedding parameters
    int embedding_dim = 128;
    float leap_threshold = 0.7f;
    float similarity_threshold = 0.6f;
    float embedding_learning_rate = 0.01f;
    
    // Dual-state evolution parameters
    float fast_update_rate = 0.1f;
    float slow_consolidation_rate = 0.01f;
    int consolidation_frequency = 100; // cycles
    
    // Context buffer parameters
    size_t max_context_size = 32;
    float context_decay_rate = 0.95f;
    float context_influence_weight = 0.3f;
    
    // Multimodal latent space parameters
    int latent_dim = 256;
    std::vector<float> audio_projection_weights;
    std::vector<float> image_projection_weights;
    std::vector<float> text_projection_weights;
    
    // Feedback loop parameters
    float self_reinforcement_rate = 0.05f;
    float thought_node_decay = 0.99f;
    int max_thought_nodes = 1000;
    
    // Logging and monitoring
    bool log_attention_entropy = true;
    bool log_output_diversity = true;
    bool log_embedding_coherence = true;
    bool log_dual_state_drift = true;
    
    // Performance tuning
    int beam_width = 8;
    int max_hops = 6;
    float leap_bias = 0.15f;
    float abstraction_threshold = 0.78f;
    
    // Legacy compatibility
    bool enable_legacy_mode = false;
    
    UCAConfig() {
        // Initialize projection weights with random values
        audio_projection_weights.resize(latent_dim * embedding_dim, 0.1f);
        image_projection_weights.resize(latent_dim * embedding_dim, 0.1f);
        text_projection_weights.resize(latent_dim * embedding_dim, 0.1f);
    }
    
    // Serialization support
    std::string to_json() const;
    void from_json(const std::string& json_str);
    
    // Validation
    bool validate() const;
    
    // Update methods for dual-state evolution
    void update_fast_params(const UCAConfig& delta);
    void consolidate_slow_params();
    float compute_drift() const;
};

/**
 * Evolution State for Dual-State System
 * 
 * Implements fast/slow parameter separation inspired by
 * hippocampal-cortical memory consolidation.
 */
struct EvolutionState {
    UCAConfig fast_params;  // Volatile, updated each session
    UCAConfig slow_params; // Persistent, consolidated nightly
    int cycle_count = 0;
    float total_drift = 0.0f;
    
    void update_cycle() {
        cycle_count++;
        total_drift += fast_params.compute_drift();
    }
    
    void consolidate() {
        // Blend fast into slow with small learning rate
        slow_params.fast_update_rate = fast_params.fast_update_rate;
        slow_params.slow_consolidation_rate = fast_params.slow_consolidation_rate;
        
        // Reset fast params to slow baseline
        fast_params = slow_params;
        total_drift = 0.0f;
    }
    
    bool should_consolidate() const {
        return cycle_count % slow_params.consolidation_frequency == 0;
    }
};

/**
 * Context Buffer for Dynamic Replay
 * 
 * Maintains recent reasoning context as ThoughtNode sequences
 * for improved continuity and coherence.
 */
struct ThoughtNode {
    std::string content;
    std::vector<float> embedding;
    float confidence;
    uint64_t timestamp;
    std::vector<uint64_t> involved_nodes;
    
    ThoughtNode(const std::string& text, const std::vector<float>& emb, float conf)
        : content(text), embedding(emb), confidence(conf) {
        timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
    }
};

class ContextBuffer {
private:
    std::deque<ThoughtNode> buffer;
    size_t max_size;
    float decay_rate;
    
public:
    ContextBuffer(size_t max_size = 32, float decay_rate = 0.95f)
        : max_size(max_size), decay_rate(decay_rate) {}
    
    void push(const ThoughtNode& thought) {
        if (buffer.size() >= max_size) {
            buffer.pop_front();
        }
        buffer.push_back(thought);
    }
    
    std::vector<uint64_t> get_recent_context() const {
        std::vector<uint64_t> context;
        for (const auto& thought : buffer) {
            context.insert(context.end(), 
                          thought.involved_nodes.begin(), 
                          thought.involved_nodes.end());
        }
        return context;
    }
    
    float compute_context_overlap(const std::vector<uint64_t>& nodes) const {
        if (buffer.empty()) return 0.0f;
        
        float total_overlap = 0.0f;
        for (const auto& thought : buffer) {
            float overlap = 0.0f;
            for (const auto& node : nodes) {
                if (std::find(thought.involved_nodes.begin(), 
                             thought.involved_nodes.end(), node) != 
                    thought.involved_nodes.end()) {
                    overlap += 1.0f;
                }
            }
            total_overlap += overlap / thought.involved_nodes.size();
        }
        
        return total_overlap / buffer.size();
    }
    
    void decay_context() {
        // Apply temporal decay to older thoughts
        for (auto& thought : buffer) {
            thought.confidence *= decay_rate;
        }
        
        // Remove very low confidence thoughts
        buffer.erase(
            std::remove_if(buffer.begin(), buffer.end(),
                [](const ThoughtNode& t) { return t.confidence < 0.1f; }),
            buffer.end());
    }
    
    size_t size() const { return buffer.size(); }
    bool empty() const { return buffer.empty(); }
};

/**
 * Attention State for Soft Graph Traversal
 * 
 * Maintains attention weights during reasoning cycles
 * for LLM-style attention mechanisms.
 */
struct AttentionState {
    std::unordered_map<uint64_t, float> attention_weights; // Using uint64_t for compatibility
    std::vector<std::unordered_map<uint64_t, float>> attention_history;
    float total_entropy = 0.0f;
    int depth = 0;
    
    void update_attention(const std::unordered_map<uint64_t, float>& new_weights) {
        attention_history.push_back(attention_weights);
        attention_weights = new_weights;
        depth++;
        
        // Compute attention entropy
        total_entropy = 0.0f;
        for (const auto& [node, weight] : attention_weights) {
            if (weight > 0.0f) {
                total_entropy -= weight * std::log2(weight);
            }
        }
    }
    
    void normalize_attention() {
        float total = 0.0f;
        for (const auto& [node, weight] : attention_weights) {
            total += weight;
        }
        
        if (total > 0.0f) {
            for (auto& [node, weight] : attention_weights) {
                weight /= total;
            }
        }
    }
    
    std::vector<uint64_t> get_top_nodes(int k = 3) const {
        std::vector<std::pair<uint64_t, float>> sorted_nodes;
        for (const auto& [node, weight] : attention_weights) {
            sorted_nodes.emplace_back(node, weight);
        }
        
        std::sort(sorted_nodes.begin(), sorted_nodes.end(),
            [](const auto& a, const auto& b) { return a.second > b.second; });
        
        std::vector<uint64_t> result;
        for (int i = 0; i < std::min(k, (int)sorted_nodes.size()); ++i) {
            result.push_back(sorted_nodes[i].first);
        }
        
        return result;
    }
};

} // namespace melvin
