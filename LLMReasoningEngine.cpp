/*
 * LLM-Style Reasoning Engine for Melvin
 * 
 * Implements soft attention-style reasoning, probabilistic output generation,
 * and embedding-aware processing inspired by transformer architectures.
 */

#include "UCAConfig.h"
#include "melvin_types.h"
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <string>
#include <cmath>
#include <algorithm>
#include <random>
#include <chrono>

namespace melvin {

/**
 * LLM-Style Reasoning Engine
 * 
 * Replaces rigid graph traversal with soft attention mechanisms
 * and probabilistic reasoning inspired by transformer architectures.
 */
class LLMReasoningEngine {
private:
    UCAConfig config;
    EvolutionState evolution_state;
    ContextBuffer context_buffer;
    std::mt19937 rng;
    
    // Graph storage references
    std::unordered_map<NodeID, Node>* nodes;
    std::unordered_map<EdgeID, Edge>* edges;
    
    // Attention and reasoning state
    AttentionState current_attention;
    std::vector<float> global_context_embedding;
    
    // Performance metrics
    float attention_entropy = 0.0f;
    float output_diversity = 0.0f;
    float embedding_coherence = 0.0f;
    
public:
    LLMReasoningEngine() : rng(std::random_device{}()) {
        global_context_embedding.resize(config.embedding_dim, 0.0f);
    }
    
    void configure(const UCAConfig& cfg) {
        config = cfg;
        context_buffer = ContextBuffer(cfg.max_context_size, cfg.context_decay_rate);
    }
    
    void set_graph_references(std::unordered_map<NodeID, Node>* node_map,
                             std::unordered_map<EdgeID, Edge>* edge_map) {
        nodes = node_map;
        edges = edge_map;
    }
    
    /**
     * Soft Graph Traversal with Attention
     * 
     * Replaces discrete edge walking with weighted attention mechanisms
     * inspired by multi-head attention in transformers.
     */
    std::vector<NodeID> soft_traverse(NodeID start, int depth = -1) {
        if (depth == -1) depth = config.max_attention_depth;
        
        std::unordered_map<NodeID, float> attention;
        attention[start] = 1.0f;
        
        // Apply context influence
        if (config.use_context_buffer && !context_buffer.empty()) {
            auto context_nodes = context_buffer.get_recent_context();
            for (const auto& node : context_nodes) {
                if (attention.find(node) == attention.end()) {
                    attention[node] = config.context_influence_weight;
                } else {
                    attention[node] += config.context_influence_weight;
                }
            }
        }
        
        // Multi-step attention propagation
        for (int d = 0; d < depth; ++d) {
            std::unordered_map<NodeID, float> next_attention;
            
            for (const auto& [node_id, weight] : attention) {
                if (nodes->find(node_id) == nodes->end()) continue;
                
                // Get connected nodes with edge weights
                auto connected = get_connected_nodes(node_id);
                
                for (const auto& [target_id, edge_weight] : connected) {
                    // Compute attention strength
                    float strength = compute_attention_strength(node_id, target_id, edge_weight);
                    
                    // Apply temperature scaling
                    strength = std::pow(strength, config.attention_temperature);
                    
                    // Accumulate attention
                    next_attention[target_id] += weight * strength;
                }
            }
            
            // Normalize attention weights
            normalize_attention_map(next_attention);
            
            // Apply dropout for regularization
            if (config.attention_dropout > 0.0f) {
                apply_attention_dropout(next_attention);
            }
            
            attention = next_attention;
        }
        
        // Update current attention state
        current_attention.update_attention(attention);
        
        // Return top nodes by attention weight
        return current_attention.get_top_nodes(config.beam_width);
    }
    
    /**
     * Probabilistic Output Generation
     * 
     * Generates outputs using soft sampling instead of deterministic chains,
     * inspired by LLM token generation with temperature and top-p sampling.
     */
    std::string generate_phrase(NodeID start) {
        std::string output;
        NodeID current = start;
        std::unordered_set<NodeID> visited;
        
        for (int i = 0; i < config.max_output_length; ++i) {
            if (nodes->find(current) == nodes->end()) break;
            
            // Add current node text to output
            const auto& node = (*nodes)[current];
            if (!output.empty()) output += " ";
            output += node.text;
            
            // Get next possible nodes
            auto next_candidates = get_connected_nodes(current);
            if (next_candidates.empty()) break;
            
            // Apply repetition penalty
            apply_repetition_penalty(next_candidates, visited);
            
            // Softmax sampling with temperature
            current = weighted_random_choice(next_candidates, config.output_temperature);
            
            // Track visited nodes for repetition penalty
            visited.insert(current);
            
            // Apply top-p filtering
            if (config.output_top_p < 1.0f) {
                current = apply_top_p_sampling(next_candidates, config.output_top_p);
            }
        }
        
        // Store generated phrase as thought node
        if (config.use_feedback_loop) {
            store_thought_node(output, current);
        }
        
        return output;
    }
    
    /**
     * Embedding-Aware Reasoning
     * 
     * Uses semantic embeddings for leap connections and analogy reasoning,
     * enabling generalization beyond explicit graph connections.
     */
    void update_embeddings() {
        if (!config.use_embeddings) return;
        
        std::unordered_map<NodeID, std::vector<float>> new_embeddings;
        
        for (auto& [node_id, node] : *nodes) {
            if (node.embedding.empty()) {
                // Initialize embedding if not present
                node.embedding.resize(config.embedding_dim);
                initialize_embedding(node.embedding);
            }
            
            // Compute new embedding based on connected nodes
            std::vector<float> aggregated_embedding(config.embedding_dim, 0.0f);
            float total_weight = 0.0f;
            
            auto connected = get_connected_nodes(node_id);
            for (const auto& [target_id, weight] : connected) {
                if (nodes->find(target_id) != nodes->end()) {
                    const auto& target_node = (*nodes)[target_id];
                    if (!target_node.embedding.empty()) {
                        for (size_t i = 0; i < config.embedding_dim; ++i) {
                            aggregated_embedding[i] += weight * target_node.embedding[i];
                        }
                        total_weight += weight;
                    }
                }
            }
            
            // Normalize and blend with current embedding
            if (total_weight > 0.0f) {
                for (size_t i = 0; i < config.embedding_dim; ++i) {
                    aggregated_embedding[i] /= total_weight;
                    node.embedding[i] = (1.0f - config.embedding_learning_rate) * node.embedding[i] +
                                       config.embedding_learning_rate * aggregated_embedding[i];
                }
            }
        }
        
        // Update embedding coherence metric
        update_embedding_coherence();
    }
    
    /**
     * Dual-State Evolution
     * 
     * Implements fast/slow parameter separation for stable learning
     * inspired by hippocampal-cortical memory consolidation.
     */
    void evolve_parameters() {
        if (!config.dual_state_evolution) return;
        
        evolution_state.update_cycle();
        
        // Update fast parameters based on recent performance
        update_fast_parameters();
        
        // Consolidate to slow parameters if needed
        if (evolution_state.should_consolidate()) {
            evolution_state.consolidate();
            std::cout << "[Evolution] Consolidated parameters after " 
                      << evolution_state.cycle_count << " cycles\n";
        }
    }
    
    /**
     * Context Buffer Management
     * 
     * Maintains recent reasoning context for improved continuity
     * and coherence across reasoning cycles.
     */
    void update_context_buffer() {
        if (!config.use_context_buffer) return;
        
        // Decay older context
        context_buffer.decay_context();
        
        // Update global context embedding
        update_global_context_embedding();
    }
    
    /**
     * Multimodal Latent Space Projection
     * 
     * Projects different sensory modalities into a unified latent space
     * for cross-modal reasoning and generation.
     */
    std::vector<float> project_to_latent(const std::vector<float>& input, int modality_type) {
        std::vector<float> result(config.latent_dim, 0.0f);
        
        const std::vector<float>* weights = nullptr;
        switch (modality_type) {
            case 0: weights = &config.audio_projection_weights; break;
            case 1: weights = &config.image_projection_weights; break;
            case 2: weights = &config.text_projection_weights; break;
            default: return result;
        }
        
        // Linear projection
        for (size_t i = 0; i < config.latent_dim; ++i) {
            for (size_t j = 0; j < input.size() && j < config.embedding_dim; ++j) {
                result[i] += input[j] * (*weights)[i * config.embedding_dim + j];
            }
        }
        
        return result;
    }
    
    // Getters for metrics
    float get_attention_entropy() const { return attention_entropy; }
    float get_output_diversity() const { return output_diversity; }
    float get_embedding_coherence() const { return embedding_coherence; }
    const UCAConfig& get_config() const { return config; }
    
    // Configuration updates
    void update_config(const UCAConfig& new_config) {
        config = new_config;
        context_buffer = ContextBuffer(config.max_context_size, config.context_decay_rate);
    }

private:
    std::unordered_map<NodeID, float> get_connected_nodes(NodeID node_id) {
        std::unordered_map<NodeID, float> connected;
        
        for (const auto& [edge_id, edge] : *edges) {
            if (edge.u == node_id) {
                float weight = compute_edge_weight(edge);
                connected[edge.v] = weight;
            }
        }
        
        return connected;
    }
    
    float compute_attention_strength(NodeID from, NodeID to, float edge_weight) {
        if (nodes->find(from) == nodes->end() || nodes->find(to) == nodes->end()) {
            return 0.0f;
        }
        
        const auto& from_node = (*nodes)[from];
        const auto& to_node = (*nodes)[to];
        
        // Base attention from edge weight
        float attention = edge_weight;
        
        // Boost attention based on semantic similarity
        if (!from_node.embedding.empty() && !to_node.embedding.empty()) {
            float similarity = cosine_similarity(from_node.embedding, to_node.embedding);
            attention *= (1.0f + similarity * config.similarity_threshold);
        }
        
        // Boost attention based on recency
        uint64_t now = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        
        float recency_factor = 1.0f;
        if (to_node.last_accessed > 0) {
            float time_diff = (now - to_node.last_accessed) / 1000.0f; // seconds
            recency_factor = std::exp(-time_diff / 60.0f); // 1-minute decay
        }
        
        attention *= recency_factor;
        
        return attention;
    }
    
    float compute_edge_weight(const Edge& edge) {
        // Base weight from edge type
        float weight = 1.0f;
        switch (edge.rel) {
            case Rel::EXACT: weight = 1.0f; break;
            case Rel::TEMPORAL: weight = 1.2f; break;
            case Rel::GENERALIZATION: weight = 1.1f; break;
            case Rel::LEAP: weight = 0.85f; break;
        }
        
        // Apply frequency weighting
        if (nodes->find(edge.u) != nodes->end() && nodes->find(edge.v) != nodes->end()) {
            const auto& u_node = (*nodes)[edge.u];
            const auto& v_node = (*nodes)[edge.v];
            weight *= std::sqrt(u_node.freq * v_node.freq);
        }
        
        return weight;
    }
    
    void normalize_attention_map(std::unordered_map<NodeID, float>& attention) {
        float total = 0.0f;
        for (const auto& [node, weight] : attention) {
            total += weight;
        }
        
        if (total > 0.0f) {
            for (auto& [node, weight] : attention) {
                weight /= total;
            }
        }
    }
    
    void apply_attention_dropout(std::unordered_map<NodeID, float>& attention) {
        std::uniform_real_distribution<float> dropout_dist(0.0f, 1.0f);
        
        for (auto& [node, weight] : attention) {
            if (dropout_dist(rng) < config.attention_dropout) {
                weight = 0.0f;
            }
        }
        
        normalize_attention_map(attention);
    }
    
    NodeID weighted_random_choice(const std::unordered_map<NodeID, float>& candidates, float temperature) {
        if (candidates.empty()) return 0;
        
        // Apply temperature scaling
        std::vector<std::pair<NodeID, float>> scaled_candidates;
        float total_weight = 0.0f;
        
        for (const auto& [node, weight] : candidates) {
            float scaled_weight = std::pow(weight, 1.0f / temperature);
            scaled_candidates.emplace_back(node, scaled_weight);
            total_weight += scaled_weight;
        }
        
        // Sample from scaled weights
        std::uniform_real_distribution<float> dist(0.0f, total_weight);
        float random_value = dist(rng);
        
        float cumulative = 0.0f;
        for (const auto& [node, weight] : scaled_candidates) {
            cumulative += weight;
            if (random_value <= cumulative) {
                return node;
            }
        }
        
        return scaled_candidates.back().first;
    }
    
    void apply_repetition_penalty(std::unordered_map<NodeID, float>& candidates, 
                                 const std::unordered_set<NodeID>& visited) {
        for (auto& [node, weight] : candidates) {
            if (visited.find(node) != visited.end()) {
                weight /= config.repetition_penalty;
            }
        }
    }
    
    NodeID apply_top_p_sampling(const std::unordered_map<NodeID, float>& candidates, float top_p) {
        // Sort candidates by weight
        std::vector<std::pair<NodeID, float>> sorted_candidates;
        for (const auto& [node, weight] : candidates) {
            sorted_candidates.emplace_back(node, weight);
        }
        
        std::sort(sorted_candidates.begin(), sorted_candidates.end(),
            [](const auto& a, const auto& b) { return a.second > b.second; });
        
        // Find cumulative probability threshold
        float cumulative_prob = 0.0f;
        size_t cutoff = 0;
        
        for (size_t i = 0; i < sorted_candidates.size(); ++i) {
            cumulative_prob += sorted_candidates[i].second;
            if (cumulative_prob >= top_p) {
                cutoff = i + 1;
                break;
            }
        }
        
        // Sample from top-p candidates
        std::uniform_int_distribution<size_t> dist(0, cutoff - 1);
        size_t index = dist(rng);
        
        return sorted_candidates[index].first;
    }
    
    void store_thought_node(const std::string& content, NodeID involved_node) {
        if (!config.use_feedback_loop) return;
        
        // Create embedding for the thought
        std::vector<float> thought_embedding(config.embedding_dim, 0.0f);
        if (nodes->find(involved_node) != nodes->end()) {
            thought_embedding = (*nodes)[involved_node].embedding;
        }
        
        // Create thought node
        ThoughtNode thought(content, thought_embedding, 1.0f);
        thought.involved_nodes.push_back(involved_node);
        
        // Add to context buffer
        context_buffer.push(thought);
        
        // Reinforce involved edges
        reinforce_used_edges(involved_node);
    }
    
    void reinforce_used_edges(NodeID node_id) {
        // Increase semantic strength of recently used nodes
        if (nodes->find(node_id) != nodes->end()) {
            auto& node = (*nodes)[node_id];
            node.semantic_strength += config.self_reinforcement_rate;
            node.last_accessed = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now().time_since_epoch()).count();
        }
    }
    
    float cosine_similarity(const std::vector<float>& a, const std::vector<float>& b) {
        if (a.size() != b.size()) return 0.0f;
        
        float dot_product = 0.0f;
        float norm_a = 0.0f;
        float norm_b = 0.0f;
        
        for (size_t i = 0; i < a.size(); ++i) {
            dot_product += a[i] * b[i];
            norm_a += a[i] * a[i];
            norm_b += b[i] * b[i];
        }
        
        if (norm_a == 0.0f || norm_b == 0.0f) return 0.0f;
        
        return dot_product / (std::sqrt(norm_a) * std::sqrt(norm_b));
    }
    
    void initialize_embedding(std::vector<float>& embedding) {
        std::uniform_real_distribution<float> dist(-0.1f, 0.1f);
        for (float& val : embedding) {
            val = dist(rng);
        }
    }
    
    void update_embedding_coherence() {
        float total_coherence = 0.0f;
        int count = 0;
        
        for (const auto& [node_id, node] : *nodes) {
            if (node.embedding.empty()) continue;
            
            auto connected = get_connected_nodes(node_id);
            float node_coherence = 0.0f;
            int connected_count = 0;
            
            for (const auto& [target_id, weight] : connected) {
                if (nodes->find(target_id) != nodes->end()) {
                    const auto& target_node = (*nodes)[target_id];
                    if (!target_node.embedding.empty()) {
                        float similarity = cosine_similarity(node.embedding, target_node.embedding);
                        node_coherence += similarity * weight;
                        connected_count++;
                    }
                }
            }
            
            if (connected_count > 0) {
                total_coherence += node_coherence / connected_count;
                count++;
            }
        }
        
        embedding_coherence = count > 0 ? total_coherence / count : 0.0f;
    }
    
    void update_fast_parameters() {
        // Update fast parameters based on recent performance
        // This is a simplified version - in practice, this would be more sophisticated
        
        float performance_factor = (attention_entropy + output_diversity + embedding_coherence) / 3.0f;
        
        // Adjust temperature based on performance
        if (performance_factor > 0.8f) {
            config.output_temperature *= 1.01f; // Increase creativity
        } else if (performance_factor < 0.5f) {
            config.output_temperature *= 0.99f; // Decrease randomness
        }
        
        // Clamp temperature
        config.output_temperature = std::max(0.1f, std::min(2.0f, config.output_temperature));
    }
    
    void update_global_context_embedding() {
        if (context_buffer.empty()) return;
        
        std::fill(global_context_embedding.begin(), global_context_embedding.end(), 0.0f);
        
        for (const auto& thought : context_buffer.buffer) {
            if (thought.embedding.size() == global_context_embedding.size()) {
                for (size_t i = 0; i < global_context_embedding.size(); ++i) {
                    global_context_embedding[i] += thought.confidence * thought.embedding[i];
                }
            }
        }
        
        // Normalize
        float total_confidence = 0.0f;
        for (const auto& thought : context_buffer.buffer) {
            total_confidence += thought.confidence;
        }
        
        if (total_confidence > 0.0f) {
            for (float& val : global_context_embedding) {
                val /= total_confidence;
            }
        }
    }
};

} // namespace melvin
