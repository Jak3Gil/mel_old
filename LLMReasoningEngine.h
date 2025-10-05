#pragma once

#include "UCAConfig.h"
#include "melvin_types.h"
#include <unordered_map>
#include <vector>
#include <string>

namespace melvin {

// Forward declarations
struct Node;
struct Edge;

/**
 * LLM-Style Reasoning Engine Header
 * 
 * Provides soft attention-style reasoning, probabilistic output generation,
 * and embedding-aware processing for Melvin's cognitive architecture.
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
    LLMReasoningEngine();
    
    void configure(const UCAConfig& cfg);
    void set_graph_references(std::unordered_map<NodeID, Node>* node_map,
                             std::unordered_map<EdgeID, Edge>* edge_map);
    
    // Core LLM-style reasoning methods
    std::vector<NodeID> soft_traverse(NodeID start, int depth = -1);
    std::string generate_phrase(NodeID start);
    void update_embeddings();
    void evolve_parameters();
    void update_context_buffer();
    
    // Multimodal projection
    std::vector<float> project_to_latent(const std::vector<float>& input, int modality_type);
    
    // Metrics and configuration
    float get_attention_entropy() const { return attention_entropy; }
    float get_output_diversity() const { return output_diversity; }
    float get_embedding_coherence() const { return embedding_coherence; }
    const UCAConfig& get_config() const { return config; }
    void update_config(const UCAConfig& new_config);

private:
    // Internal helper methods
    std::unordered_map<NodeID, float> get_connected_nodes(NodeID node_id);
    float compute_attention_strength(NodeID from, NodeID to, float edge_weight);
    float compute_edge_weight(const Edge& edge);
    void normalize_attention_map(std::unordered_map<NodeID, float>& attention);
    void apply_attention_dropout(std::unordered_map<NodeID, float>& attention);
    NodeID weighted_random_choice(const std::unordered_map<NodeID, float>& candidates, float temperature);
    void apply_repetition_penalty(std::unordered_map<NodeID, float>& candidates, 
                                 const std::unordered_set<NodeID>& visited);
    NodeID apply_top_p_sampling(const std::unordered_map<NodeID, float>& candidates, float top_p);
    void store_thought_node(const std::string& content, NodeID involved_node);
    void reinforce_used_edges(NodeID node_id);
    float cosine_similarity(const std::vector<float>& a, const std::vector<float>& b);
    void initialize_embedding(std::vector<float>& embedding);
    void update_embedding_coherence();
    void update_fast_parameters();
    void update_global_context_embedding();
};

} // namespace melvin
