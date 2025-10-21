#pragma once

#include "atomic_graph.h"
#include <vector>
#include <string>
#include <unordered_map>

namespace melvin {
namespace uca {

// ============================================================================
// BIOLOGICAL ANALOG: Cortical Memory Systems
// ============================================================================

/**
 * Extended relation types matching cortical connectivity patterns
 * 
 * Biological mapping:
 * - EXACT edges → Hippocampal explicit memory (episodic facts)
 * - LEAP edges → Cortical associations (semantic connections)
 * - Temporal edges → Medial temporal lobe (sequence learning)
 * - Spatial edges → Parietal cortex (where pathway)
 * - Causal edges → Prefrontal cortex (planning/reasoning)
 */
enum ExtendedRelation : uint8_t {
    // Existing relations (from atomic_graph.h)
    INSTANCE_OF_EXT = 0,
    CO_OCCURS_WITH_EXT = 1,
    OBSERVED_AS_EXT = 2,
    NEAR_EXT = 3,
    TEMPORAL_NEXT_EXT = 4,
    NAMED_EXT = 5,
    
    // NEW: Semantic relations (IT cortex)
    PART_OF = 6,              // "wheel" PART_OF "car"
    HAS_PROPERTY = 7,         // "fire" HAS_PROPERTY "hot"
    USED_FOR = 8,             // "tool" USED_FOR "action"
    SIMILAR_TO = 9,           // Category similarity
    
    // NEW: Causal relations (PFC)
    CAUSES = 10,              // "heat" CAUSES "melting"
    ENABLES = 11,             // "key" ENABLES "door_open"
    PREVENTS = 12,            // "water" PREVENTS "fire"
    
    // NEW: Predictive relations (forward model)
    EXPECTS = 13,             // "dark_cloud" EXPECTS "rain"
    FOLLOWS = 14,             // Temporal prediction
    CONTRADICTS = 15,         // Inhibitory connection
    
    // NEW: Goal relations (motivational)
    DESIRES = 16,             // Goal-driven
    AVOIDS = 17,              // Aversive
    REQUIRES = 18             // Dependency
};

/**
 * Node categories (mimics cortical specialization)
 */
enum NodeCategory : uint8_t {
    PERCEPTION_NODE = 0,      // Sensory input (V1/A1)
    OBJECT_NODE = 1,          // Recognized objects (IT)
    CONCEPT_NODE = 2,         // Abstract concepts (aTL)
    ACTION_NODE = 3,          // Motor plans (M1/PM)
    GOAL_NODE = 4,            // Motivational states (OFC)
    SCENE_NODE = 5,           // Contextual frames (RSC)
    TEMPORAL_NODE = 6,        // Time markers (MTL)
    ABSTRACT_NODE = 7         // High-level thoughts (PFC)
};

// ============================================================================
// CORTICAL GRAPH - Enhanced Knowledge Representation
// ============================================================================

/**
 * MelvinGraph: Extended AtomicGraph with cortical-inspired operations
 * 
 * Adds:
 * - Hierarchical node categories
 * - Extended relation types
 * - Hebbian learning (fire together → wire together)
 * - Edge decay (forgetting)
 * - LEAP discovery (pattern-based inference)
 * - Activation spreading (energy field)
 * - Cortical routing (anatomical constraints)
 */
class MelvinGraph {
public:
    MelvinGraph();
    ~MelvinGraph();
    
    // ========================================================================
    // CORTICAL MEMORY OPERATIONS
    // ========================================================================
    
    /**
     * HIPPOCAMPUS ANALOG: Create explicit episodic memory
     * Links: who, what, where, when
     */
    struct EpisodicMemory {
        uint64_t agent_node;      // Who
        uint64_t action_node;     // What
        uint64_t object_node;     // Object
        uint64_t location_node;   // Where
        uint64_t time_node;       // When
        float confidence;
    };
    
    void store_episode(const EpisodicMemory& episode);
    std::vector<EpisodicMemory> recall_episodes(uint64_t query_node, int max_results = 10);
    
    /**
     * SEMANTIC MEMORY ANALOG: Abstract knowledge (no temporal context)
     */
    void add_semantic_fact(
        const std::string& subject,
        ExtendedRelation relation,
        const std::string& object,
        float confidence = 1.0f
    );
    
    /**
     * WORKING MEMORY ANALOG: Temporary high-activation nodes
     */
    void activate_node(uint64_t node_id, float activation_energy);
    std::vector<uint64_t> get_active_nodes(float threshold = 0.5f);
    void decay_activations(float decay_rate = 0.95f);
    
    // ========================================================================
    // HEBBIAN LEARNING
    // ========================================================================
    
    /**
     * "Neurons that fire together, wire together"
     * Strengthens edges between co-active nodes
     */
    void hebbian_update(const std::vector<uint64_t>& co_active_nodes);
    
    /**
     * Edge decay (forgetting)
     * Weakens unused connections over time
     */
    void decay_edges(float decay_rate = 0.99f);
    
    /**
     * LEAP discovery: Find transitive shortcuts
     * If A→B and B→C, infer A→C
     */
    void discover_leaps(int max_hops = 3);
    
    // ========================================================================
    // ACTIVATION SPREADING (Neural dynamics)
    // ========================================================================
    
    /**
     * Spread activation from source nodes through graph
     * Mimics cortical excitatory connections
     */
    std::unordered_map<uint64_t, float> spread_activation(
        const std::vector<uint64_t>& source_nodes,
        int max_hops = 5,
        float decay_per_hop = 0.8f
    );
    
    /**
     * Attractor dynamics: Let activations settle
     * Hopfield-like convergence to stable state
     */
    void settle_activations(int max_iterations = 10);
    
    // ========================================================================
    // CORTICAL ROUTING (Anatomical constraints)
    // ========================================================================
    
    /**
     * Find paths respecting cortical hierarchy
     * E.g., Perception → IT → PFC, not Perception → PFC directly
     */
    struct CorticalPath {
        std::vector<uint64_t> nodes;
        std::vector<ExtendedRelation> relations;
        float confidence;
    };
    
    std::vector<CorticalPath> find_cortical_paths(
        uint64_t from,
        uint64_t to,
        int max_hops = 10
    );
    
    /**
     * Predict next node given current activation state
     * Uses graph structure + temporal patterns
     */
    std::vector<std::pair<uint64_t, float>> predict_next_nodes(
        uint64_t current_node,
        int top_k = 5
    );
    
    // ========================================================================
    // GRAPH QUERIES
    // ========================================================================
    
    /**
     * Get all nodes of specific category
     */
    std::vector<uint64_t> get_nodes_by_category(NodeCategory category);
    
    /**
     * Get all edges of specific relation
     */
    std::vector<Edge> get_edges_by_relation(ExtendedRelation relation);
    
    /**
     * Find nodes within activation radius
     */
    std::vector<uint64_t> get_neighbors(
        uint64_t node_id,
        int hops = 1,
        ExtendedRelation filter_relation = ExtendedRelation(255) // 255 = any
    );
    
    /**
     * Concept centrality (how connected is this node?)
     */
    float compute_centrality(uint64_t node_id);
    
    /**
     * Novelty score (how unusual is this connection?)
     */
    float compute_novelty(uint64_t node_a, uint64_t node_b);
    
    // ========================================================================
    // PERSISTENCE
    // ========================================================================
    
    bool load(const std::string& nodes_path, const std::string& edges_path);
    bool save(const std::string& nodes_path, const std::string& edges_path);
    
    // ========================================================================
    // STATISTICS
    // ========================================================================
    
    struct GraphStats {
        size_t total_nodes;
        size_t total_edges;
        size_t exact_edges;
        size_t leap_edges;
        std::unordered_map<NodeCategory, size_t> nodes_by_category;
        std::unordered_map<ExtendedRelation, size_t> edges_by_relation;
        float avg_degree;
        float clustering_coefficient;
    };
    
    GraphStats get_stats() const;
    void print_stats() const;
    
    // ========================================================================
    // ACCESS TO UNDERLYING GRAPH
    // ========================================================================
    
    AtomicGraph* get_atomic_graph() { return graph_.get(); }
    
private:
    // ========================================================================
    // INTERNAL STRUCTURES
    // ========================================================================
    
    struct NodeMetadata {
        NodeCategory category;
        float activation;           // Current activation level
        float baseline_activation;  // Resting potential
        int access_count;           // How often accessed
        double last_access_time;    // For decay
    };
    
    struct EdgeMetadata {
        float strength;             // Synaptic weight
        int reinforcement_count;    // Hebbian accumulation
        double last_reinforced;     // For decay
        bool is_leap;               // Inferred vs explicit
    };
    
    // ========================================================================
    // STATE
    // ========================================================================
    
    std::unique_ptr<AtomicGraph> graph_;
    
    std::unordered_map<uint64_t, NodeMetadata> node_metadata_;
    std::unordered_map<std::string, EdgeMetadata> edge_metadata_; // "a_b" key
    
    // Working memory (high-activation nodes)
    std::vector<uint64_t> working_memory_;
    
    // Statistics
    uint64_t total_episodes_stored_;
    uint64_t total_leaps_discovered_;
    
    // ========================================================================
    // HELPER FUNCTIONS
    // ========================================================================
    
    std::string edge_key(uint64_t a, uint64_t b) const;
    NodeMetadata& get_or_create_metadata(uint64_t node_id);
    EdgeMetadata& get_or_create_edge_metadata(uint64_t a, uint64_t b);
    
    double get_current_time();
};

} // namespace uca
} // namespace melvin


