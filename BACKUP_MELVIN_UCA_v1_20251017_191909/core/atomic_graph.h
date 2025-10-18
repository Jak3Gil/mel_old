#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <unordered_map>

namespace melvin {

// ============================================================================
// ATOMIC GRAPH - Minimal Binary Storage for Vision + Language
// ============================================================================

/**
 * Node: 40 bytes (fixed size, binary-compatible)
 */
struct Node {
    uint64_t id;        // 8 bytes - unique identifier
    uint8_t type;       // 1 byte - 0=concept, 1=instance, 2=feature
    char label[31];     // 31 bytes - null-terminated string
    
    Node() : id(0), type(0) {
        label[0] = '\0';
    }
    
    Node(uint64_t node_id, uint8_t node_type, const std::string& node_label)
        : id(node_id), type(node_type) {
        std::strncpy(label, node_label.c_str(), 30);
        label[30] = '\0';  // Ensure null-termination
    }
} __attribute__((packed));

/**
 * Edge: 24 bytes (fixed size, binary-compatible)
 */
struct Edge {
    uint64_t a;         // 8 bytes - source node id
    uint64_t b;         // 8 bytes - destination node id
    float w;            // 4 bytes - weight (co-occurrence count or strength)
    uint8_t rel;        // 1 byte - relation type
    uint8_t _pad[3];    // 3 bytes - padding for alignment
    
    Edge() : a(0), b(0), w(0.0f), rel(0) {
        _pad[0] = _pad[1] = _pad[2] = 0;
    }
    
    Edge(uint64_t from, uint64_t to, uint8_t relation, float weight = 1.0f)
        : a(from), b(to), w(weight), rel(relation) {
        _pad[0] = _pad[1] = _pad[2] = 0;
    }
} __attribute__((packed));

/**
 * Relation types (1 byte enum)
 */
enum Relation : uint8_t {
    INSTANCE_OF = 0,      // instance → concept
    CO_OCCURS_WITH = 1,   // spatial co-occurrence (EXACT)
    OBSERVED_AS = 2,      // observation → concept
    NEAR = 3,             // spatial proximity
    TEMPORAL_NEXT = 4,    // temporal sequence (LEAP)
    NAMED = 5             // label assignment
};

// ============================================================================
// ATOMIC GRAPH CLASS
// ============================================================================

/**
 * Minimal binary graph for unified vision + language memory
 * 
 * Features:
 * - 40 bytes per node (fixed size)
 * - 24 bytes per edge (fixed size)
 * - Binary persistence (fast I/O)
 * - Weighted edges (co-occurrence tracking)
 * - No dynamic allocations per operation
 * - Target: <25 MB for 1M edges
 */
class AtomicGraph {
public:
    AtomicGraph();
    ~AtomicGraph();
    
    // ========================================================================
    // NODE OPERATIONS
    // ========================================================================
    
    /**
     * Get existing node ID or create new one
     * Returns: node ID
     */
    uint64_t get_or_create_node(const std::string& label, uint8_t type);
    
    /**
     * Get node by ID
     */
    const Node* get_node(uint64_t id) const;
    
    /**
     * Find nodes by label substring
     */
    std::vector<uint64_t> find_nodes(const std::string& substring) const;
    
    // ========================================================================
    // EDGE OPERATIONS
    // ========================================================================
    
    /**
     * Add edge or increment weight if exists
     * 
     * @param a Source node ID
     * @param b Destination node ID
     * @param rel Relation type
     * @param inc Weight increment (default 1.0)
     */
    void add_edge(uint64_t a, uint64_t b, uint8_t rel, float inc = 1.0f);
    
    /**
     * Decay all edge weights by factor (for temporal forgetting)
     * 
     * @param factor Decay multiplier (e.g., 0.99)
     */
    void decay_edges(float factor);
    
    /**
     * Get neighbors of a node (optionally filtered by relation)
     * 
     * @param id Node ID
     * @param rel_filter Relation type filter (255 = all relations)
     * @return Vector of neighbor node IDs
     */
    std::vector<uint64_t> neighbors(uint64_t id, uint8_t rel_filter = 255) const;
    
    /**
     * Get edge weight between two nodes
     */
    float get_edge_weight(uint64_t a, uint64_t b, uint8_t rel) const;
    
    // ========================================================================
    // PERSISTENCE
    // ========================================================================
    
    /**
     * Save graph to binary files
     */
    void save(const std::string& path_nodes, const std::string& path_edges) const;
    
    /**
     * Load graph from binary files
     */
    void load(const std::string& path_nodes, const std::string& path_edges);
    
    // ========================================================================
    // STATISTICS
    // ========================================================================
    
    size_t node_count() const { return nodes_.size(); }
    size_t edge_count() const { return edges_.size(); }
    
    void print_stats() const;
    
    /**
     * Get total graph weight (sum of all edge weights)
     */
    uint64_t total_weight() const;
    
private:
    std::unordered_map<std::string, uint64_t> label_to_id_;
    std::vector<Node> nodes_;
    std::vector<Edge> edges_;
    uint64_t next_id_ = 1;  // Sequential ID generation (starts at 1)
    
    // Helper: Find edge index
    int find_edge_index(uint64_t a, uint64_t b, uint8_t rel) const;
};

} // namespace melvin

