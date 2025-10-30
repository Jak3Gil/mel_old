#ifndef ACTIVATION_FIELD_UNIFIED_H
#define ACTIVATION_FIELD_UNIFIED_H

#include <unordered_map>
#include <vector>
#include <queue>
#include <mutex>
#include <atomic>
#include <chrono>
#include <memory>

namespace melvin {
namespace fields {

/**
 * @brief Global Activation Field - Unifies all modalities
 * 
 * Replaces the 18-step serial pipeline with continuous parallel fields.
 * All modalities (vision, audio, text, motor) write to the same field.
 * Reasoning, attention, and output operate asynchronously on this field.
 */

// Message types for async communication between fields
enum class MessageType {
    SENSORY_INPUT,      // Raw sensory data
    ATTENTION_SHIFT,    // Attention weights changed
    PREDICTION_ERROR,   // Prediction vs reality mismatch
    GOAL_UPDATE,        // New goal or sub-goal
    MOTOR_COMMAND,      // Output action
    CONSOLIDATION,      // Memory consolidation trigger
    EVOLUTION,          // Genome update trigger
    REFLECTION          // Meta-cognitive query
};

// Message payload for inter-field communication
struct FieldMessage {
    MessageType type;
    int source_node_id;
    int target_node_id;
    float energy;
    float confidence;
    std::vector<float> data;  // Context-specific payload
    std::chrono::high_resolution_clock::time_point timestamp;
    
    FieldMessage(MessageType t, int src, int tgt, float e, float c = 1.0f) 
        : type(t), source_node_id(src), target_node_id(tgt), 
          energy(e), confidence(c),
          timestamp(std::chrono::high_resolution_clock::now()) {}
};

// Working memory buffer - maintains 4-7 active concepts
struct WorkingContext {
    struct ActiveConcept {
        int node_id;
        float activation;
        float salience;  // Importance score
        std::chrono::high_resolution_clock::time_point last_refresh;
        std::vector<float> embedding;
        
        ActiveConcept(int id, float act, float sal, const std::vector<float>& emb)
            : node_id(id), activation(act), salience(sal), 
              last_refresh(std::chrono::high_resolution_clock::now()),
              embedding(emb) {}
    };
    
    std::vector<ActiveConcept> concepts;  // Max 7 concepts
    mutable std::mutex buffer_mutex;
    
    // Add/update concept in working memory
    void update_concept(int node_id, float activation, float salience, 
                       const std::vector<float>& embedding);
    
    // Decay all concepts by time
    void decay_by_time(float decay_rate);
    
    // Get most active concepts
    std::vector<int> get_active_nodes(size_t max_count = 7) const;
    
    // Compute working context vector (mean of all active embeddings)
    std::vector<float> get_context_vector();
};

// Multi-hop context propagation
struct ContextHorizon {
    struct HopNode {
        int node_id;
        float activation;
        int hop_distance;  // Distance from origin
        float path_strength;  // Product of edge weights
    };
    
    // Propagate activation through graph up to max_hops
    std::vector<HopNode> propagate(
        int origin_node,
        const std::unordered_map<int, std::vector<std::pair<int, float>>>& graph,
        int max_hops = 3,
        float threshold = 0.01f
    );
    
    // Compute context vector from multi-hop neighborhood
    std::vector<float> compute_context_vector(
        const std::vector<HopNode>& neighborhood,
        const std::unordered_map<int, std::vector<float>>& embeddings
    );
};

// Hybrid edge system - symbolic + embedding-based
struct HybridEdge {
    int from_node;
    int to_node;
    
    // Symbolic component
    enum class Type { EXACT, LEAP, TEMPORAL, CAUSAL, VISUOMOTOR } type;
    float symbolic_weight;
    
    // Embedding component
    float embedding_similarity;  // Cosine similarity of node embeddings
    
    // Combined weight
    float get_effective_weight(float symbolic_bias = 0.7f) const {
        return symbolic_bias * symbolic_weight + 
               (1.0f - symbolic_bias) * embedding_similarity;
    }
    
    HybridEdge(int from, int to, Type t, float sw, float es)
        : from_node(from), to_node(to), type(t), 
          symbolic_weight(sw), embedding_similarity(es) {}
};

// Hierarchical temporal memory layer
struct TemporalHierarchy {
    enum class Level {
        FRAMES,      // 100ms - 5s (perception)
        SCENES,      // 5s - 2min (events)
        EPISODES,    // 2min+ (sequences)
        NARRATIVES   // Long-term themes
    };
    
    struct TemporalNode {
        Level level;
        std::vector<int> constituent_nodes;  // Nodes that form this chunk
        std::vector<float> summary_embedding;  // Compressed representation
        std::chrono::high_resolution_clock::time_point start_time;
        std::chrono::high_resolution_clock::time_point end_time;
        float coherence;  // How well it fits together
    };
    
    std::unordered_map<int, TemporalNode> temporal_nodes;
    
    // Compress a sequence into next level
    int compress_sequence(const std::vector<int>& node_ids, Level from_level);
    
    // Retrieve relevant temporal context
    std::vector<int> retrieve_temporal_context(
        std::chrono::high_resolution_clock::time_point query_time,
        Level level,
        int max_results = 5
    );
};

// Main unified activation field
class UnifiedActivationField {
public:
    UnifiedActivationField();
    ~UnifiedActivationField();
    
    // Core field operations
    void inject_energy(int node_id, float energy, const std::vector<float>& embedding);
    float get_activation(int node_id) const;
    void set_activation(int node_id, float activation);
    
    // Continuous dynamics (called at 10-30 Hz)
    void tick(float dt);
    
    // Message queue system for async communication
    void post_message(const FieldMessage& msg);
    std::vector<FieldMessage> drain_messages(MessageType type);
    
    // Working context
    WorkingContext& get_working_context() { return working_context_; }
    const WorkingContext& get_working_context() const { return working_context_; }
    
    // Context propagation
    std::vector<float> compute_global_context(int origin_node, int max_hops = 3);
    
    // Hybrid edges
    void add_edge(int from, int to, HybridEdge::Type type, float weight,
                  const std::vector<float>& from_emb,
                  const std::vector<float>& to_emb);
    const std::vector<HybridEdge>& get_edges_from(int node_id) const;
    
    // Temporal hierarchy
    TemporalHierarchy& get_temporal_hierarchy() { return temporal_hierarchy_; }
    
    // Cross-modal binding
    void bind_cross_modal(int text_node, int vision_node, int motor_node,
                         float temporal_overlap);
    
    // Reflection and meta-cognition
    void trigger_reflection(const std::string& query);
    std::vector<std::string> get_reflections();
    
    // Statistics
    size_t get_active_node_count() const;
    float get_total_energy() const;
    float get_coherence() const;  // Measure of field stability
    
private:
    // Activation field
    std::unordered_map<int, float> activations_;
    std::unordered_map<int, std::vector<float>> embeddings_;
    
    // Graph structure (hybrid edges)
    std::unordered_map<int, std::vector<HybridEdge>> edges_out_;
    std::unordered_map<int, std::vector<HybridEdge>> edges_in_;
    
    // Message queues for async operation
    std::queue<FieldMessage> message_queue_;
    std::mutex queue_mutex_;
    
    // Sub-systems
    WorkingContext working_context_;
    ContextHorizon context_horizon_;
    TemporalHierarchy temporal_hierarchy_;
    
    // Reflection system
    std::vector<std::string> reflections_;
    std::mutex reflection_mutex_;
    
    // Continuous dynamics
    void decay_activations(float dt);
    void spread_activation(float dt);
    void update_working_context(float dt);
    void process_messages();
    
    // Hopfield dynamics integration
    void hopfield_update(float dt);
    
    // Coherence computation
    float compute_field_coherence() const;
    
    // Cross-modal binding strength
    float compute_binding_strength(int node_a, int node_b, float temporal_overlap);
    
    // Energy conservation
    std::atomic<float> total_energy_;
    float max_total_energy_ = 1000.0f;
    
    // Thread safety
    mutable std::mutex field_mutex_;
    
    // Timing
    std::chrono::high_resolution_clock::time_point last_tick_;
};

} // namespace fields
} // namespace melvin

#endif // ACTIVATION_FIELD_UNIFIED_H

