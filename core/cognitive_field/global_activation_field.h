#ifndef GLOBAL_ACTIVATION_FIELD_H
#define GLOBAL_ACTIVATION_FIELD_H

#include <vector>
#include <unordered_map>
#include <mutex>
#include <cmath>

namespace melvin {
namespace cognitive_field {

/**
 * Global Activation Field
 * 
 * Unified continuous field where all modalities (vision, audio, language, motor)
 * write their activations. Provides:
 * - Multi-modal resonance (cross-modal binding)
 * - Continuous decay and diffusion
 * - Embedding-based similarity
 * - Energy-driven dynamics
 * 
 * This replaces separate per-modality activation tracking with a single
 * shared substrate for emergent coherence.
 */
class GlobalActivationField {
public:
    GlobalActivationField(size_t embedding_dim = 128);
    
    // ========================================================================
    // Core Activation Interface
    // ========================================================================
    
    /**
     * Inject energy into a node from any modality
     * @param node_id - Node to activate
     * @param energy - Energy to inject
     * @param embedding - Node's semantic embedding (for similarity)
     * @param modality - Source modality (0=text, 1=vision, 2=audio, 3=motor)
     */
    void inject_energy(int node_id, float energy, 
                      const std::vector<float>& embedding, int modality);
    
    /**
     * Get current activation level of a node
     */
    float get_activation(int node_id) const;
    
    /**
     * Get current energy of a node
     */
    float get_energy(int node_id) const;
    
    /**
     * Get embedding of a node
     */
    std::vector<float> get_embedding(int node_id) const;
    
    // ========================================================================
    // Continuous Dynamics (called at 30Hz by reasoning field)
    // ========================================================================
    
    /**
     * Update field dynamics (decay, diffusion, resonance)
     * @param dt - Time step in seconds (e.g., 0.033 for 30Hz)
     */
    void update(float dt);
    
    /**
     * Spread activation from source node to neighbors
     * @param source_id - Source node
     * @param neighbor_ids - Neighbor nodes
     * @param edge_weights - Weights of connections
     * @param spread_rate - How much energy flows (0-1)
     */
    void spread_activation(int source_id, 
                          const std::vector<int>& neighbor_ids,
                          const std::vector<float>& edge_weights,
                          float spread_rate);
    
    // ========================================================================
    // Multi-Modal Resonance
    // ========================================================================
    
    /**
     * Compute cross-modal binding strength between two nodes
     * Uses temporal overlap + activation co-occurrence + embedding similarity
     */
    float compute_binding_strength(int node_a, int node_b) const;
    
    /**
     * Find all nodes with high co-activation (potential bindings)
     * @param threshold - Minimum activation for both nodes
     * @return pairs of (node_id, binding_strength)
     */
    std::vector<std::pair<int, float>> find_resonant_nodes(
        int query_node, float threshold = 0.3f) const;
    
    // ========================================================================
    // Context Integration (multi-hop)
    // ========================================================================
    
    /**
     * Compute global context vector from all active nodes
     * Weighted by activation level
     */
    std::vector<float> compute_context_vector() const;
    
    /**
     * Get top-K most active nodes
     */
    std::vector<int> get_top_active_nodes(size_t k) const;
    
    /**
     * Propagate context through multi-hop neighborhood
     * @param seed_node - Starting node
     * @param hops - Number of hops (3-5 recommended)
     * @return context-weighted node activations within horizon
     */
    std::unordered_map<int, float> propagate_context(int seed_node, int hops) const;
    
    // ========================================================================
    // Working Context Buffer (4-7 active concepts)
    // ========================================================================
    
    /**
     * Maintain a working memory buffer of top concepts
     * Decays unless reinforced
     */
    struct WorkingConcept {
        int node_id;
        float activation;
        float decay_rate;
        std::chrono::high_resolution_clock::time_point last_update;
    };
    
    std::vector<WorkingConcept> get_working_buffer() const;
    void update_working_buffer();
    
    // ========================================================================
    // Embedding-Based Similarity
    // ========================================================================
    
    /**
     * Compute cosine similarity between two node embeddings
     */
    float cosine_similarity(int node_a, int node_b) const;
    
    /**
     * Find nearest neighbors by embedding similarity
     */
    std::vector<std::pair<int, float>> find_similar_nodes(
        int query_node, size_t k, float min_similarity = 0.5f) const;
    
    // ========================================================================
    // Statistics
    // ========================================================================
    
    struct Stats {
        size_t total_nodes;
        size_t active_nodes;  // Activation > threshold
        float total_energy;
        float avg_activation;
        float max_activation;
        float resonance_count;  // Cross-modal bindings
    };
    Stats get_stats() const;
    
    void reset();
    
private:
    size_t embedding_dim_;
    
    // Node state
    struct NodeState {
        float activation;
        float energy;
        std::vector<float> embedding;
        int modality;  // Source modality
        std::chrono::high_resolution_clock::time_point last_active;
        
        // For resonance detection
        float activation_history[10];  // Ring buffer
        int history_index;
    };
    
    std::unordered_map<int, NodeState> nodes_;
    mutable std::mutex nodes_mutex_;
    
    // Working buffer
    static constexpr size_t WORKING_BUFFER_SIZE = 7;
    std::vector<WorkingConcept> working_buffer_;
    
    // Parameters (can be genome-controlled)
    float decay_rate_ = 0.95f;
    float spread_rate_ = 0.3f;
    float min_activation_ = 0.01f;
    float resonance_threshold_ = 0.5f;
    
    // Helper functions
    float compute_cosine_similarity(const std::vector<float>& a, 
                                   const std::vector<float>& b) const;
    void update_activation_history(NodeState& node);
};

} // namespace cognitive_field
} // namespace melvin

#endif // GLOBAL_ACTIVATION_FIELD_H

