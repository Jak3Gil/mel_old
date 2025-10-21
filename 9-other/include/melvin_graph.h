#pragma once

#include "melvin_types.h"
#include <vector>
#include <unordered_map>
#include <string>
#include <string_view>
#include <initializer_list>
#include <span>

namespace melvin {

/**
 * AtomicGraph - Hippocampus + Cortical Memory
 * 
 * Biological analog:
 * - Hippocampus: Episodic memory (frames, temporal sequences)
 * - Cortical networks: Semantic memory (concepts, relations)
 * - Synaptic plasticity: Edge reinforcement and decay
 * 
 * Responsibilities:
 * - Compact persistent storage of nodes and edges
 * - O(1) node lookup by label or hash
 * - Edge "chemistry": reinforcement, decay, LEAP formation
 * - Fast neighborhood queries with relation filtering
 */
class AtomicGraph {
public:
    AtomicGraph();
    ~AtomicGraph();
    
    // ========================================================================
    // NODE CREATION (O(1) with hash map)
    // ========================================================================
    
    /**
     * Get or create concept node by label
     * @param label Concept name (e.g., "fire", "cup", "person")
     * @return Node ID
     */
    uint64_t get_or_create_concept(std::string_view label);
    
    /**
     * Get or create visual patch node
     * @param frame_id Frame identifier
     * @param px Patch X coordinate
     * @param py Patch Y coordinate
     * @return Node ID (hashed from frame_id + coordinates)
     */
    uint64_t get_or_create_visual_patch(uint64_t frame_id, int px, int py);
    
    /**
     * Create frame node
     * @param frame_meta Frame metadata
     * @return Frame node ID
     */
    uint64_t create_frame(const FrameMeta& frame_meta);
    
    /**
     * Create object node
     * @param center_x Object center X
     * @param center_y Object center Y
     * @return Object node ID
     */
    uint64_t create_object(int center_x, int center_y);
    
    // ========================================================================
    // EDGE CHEMISTRY (Synaptic plasticity)
    // ========================================================================
    
    /**
     * Add edge or increment weight (Hebbian: "fire together, wire together")
     * @param a Source node
     * @param b Target node
     * @param r Relation type
     * @param inc Weight increment (default: 0.1)
     */
    void add_or_bump_edge(uint64_t a, uint64_t b, Rel r, float inc = constants::REINFORCE_ETA);
    
    /**
     * Decay all edge weights (forgetting)
     * @param seconds Elapsed time
     * @param lambda Decay rate constant (default: DECAY_LAMBDA)
     */
    void decay_edges(float seconds, float lambda = constants::DECAY_LAMBDA);
    
    /**
     * Reinforce specific edge (Hebbian reinforcement)
     * @param a Source node
     * @param b Target node
     * @param r Relation type
     * @param eta Learning rate (default: REINFORCE_ETA)
     */
    void reinforce(uint64_t a, uint64_t b, Rel r, float eta = constants::REINFORCE_ETA);
    
    /**
     * Add temporal next relation (sequence learning)
     * @param a Previous node
     * @param b Next node
     */
    void add_temp_next(uint64_t a, uint64_t b);
    
    /**
     * Maybe form LEAP shortcut (transitive inference)
     * Forms A→C if A→B→C exists and error_delta suggests utility
     * 
     * @param a Start node
     * @param b End node
     * @param error_delta Prediction error reduction
     * @param threshold Min error to justify LEAP (default: LEAP_ERROR_THRESHOLD)
     * @return true if LEAP formed
     */
    bool maybe_form_leap(uint64_t a, uint64_t b, float error_delta, 
                         float threshold = constants::LEAP_ERROR_THRESHOLD);
    
    // ========================================================================
    // QUERIES
    // ========================================================================
    
    /**
     * Get neighbors with relation filtering
     * @param id Node to query
     * @param rels Allowed relation types
     * @param k Maximum neighbors to return
     * @return List of neighbor node IDs
     */
    std::vector<uint64_t> neighbors(uint64_t id, 
                                    std::initializer_list<Rel> rels,
                                    size_t k = 64) const;
    
    /**
     * Get all neighbors (any relation)
     * @param id Node to query
     * @param k Maximum neighbors
     * @return List of neighbor node IDs
     */
    std::vector<uint64_t> all_neighbors(uint64_t id, size_t k = 64) const;
    
    /**
     * Get edge weight
     * @param a Source node
     * @param b Target node
     * @param r Relation type
     * @return Edge weight (0 if not exists)
     */
    float get_edge_weight(uint64_t a, uint64_t b, Rel r) const;
    
    /**
     * Check if node exists
     */
    bool has_node(uint64_t id) const;
    
    /**
     * Get node
     */
    const Node* get_node(uint64_t id) const;
    
    // ========================================================================
    // PERSISTENCE
    // ========================================================================
    
    /**
     * Save graph to disk
     * @param nodes_path Path for nodes file
     * @param edges_path Path for edges file
     * @return true if successful
     */
    bool save(const std::string& nodes_path, const std::string& edges_path) const;
    
    /**
     * Load graph from disk
     * @param nodes_path Path for nodes file
     * @param edges_path Path for edges file
     * @return true if successful
     */
    bool load(const std::string& nodes_path, const std::string& edges_path);
    
    // ========================================================================
    // STATISTICS
    // ========================================================================
    
    size_t node_count() const { return nodes_.size(); }
    size_t edge_count() const { return edges_.size(); }
    
    void print_stats() const;
    
private:
    // Storage
    std::unordered_map<uint64_t, Node> nodes_;
    std::vector<Edge> edges_;
    
    // Fast lookups
    std::unordered_map<std::string, uint64_t> label_to_id_;
    std::unordered_map<uint64_t, std::vector<size_t>> adjacency_; // node_id -> edge indices
    
    // Edge key for deduplication
    std::string edge_key(uint64_t a, uint64_t b, Rel r) const;
    std::unordered_map<std::string, size_t> edge_index_; // edge_key -> edge index
    
    // ID generation
    uint64_t next_node_id_;
    uint64_t next_object_id_;
};

} // namespace melvin


