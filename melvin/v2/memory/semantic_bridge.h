#pragma once

#include "../core/types_v2.h"
#include "../../core/atomic_graph.h"
#include <vector>
#include <memory>
#include <string>

namespace melvin::v2::memory {

// ============================================================================
// SEMANTIC BRIDGE - V2 Interface to V1 AtomicGraph
// ============================================================================

/**
 * SemanticBridge provides v2-compatible access to v1's AtomicGraph.
 * 
 * Key features:
 * - Zero-cost access to 4.29M edges (no data migration!)
 * - V2-style API wrapping v1 methods
 * - Read-only mode for safe migration
 * - Query optimization (caching)
 * - Statistics tracking
 * 
 * This preserves all v1 knowledge while enabling v2 reasoning.
 */
class SemanticBridge {
public:
    struct Config {
        bool readonly_mode;          // If true, no writes to v1 graph
        bool enable_caching;         // Cache frequent queries
        size_t cache_size;           // LRU cache size
        float min_confidence;        // Filter edges below this
        bool verbose;                // Debug logging
        
        Config()
            : readonly_mode(false), enable_caching(true), cache_size(1000),
              min_confidence(0.3f), verbose(false) {}
    };
    
    /**
     * Construct with existing v1 AtomicGraph
     * - Does NOT take ownership (v1 graph managed externally)
     */
    explicit SemanticBridge(melvin::AtomicGraph* v1_graph, const Config& config = Config());
    ~SemanticBridge();
    
    // ========================================================================
    // CONCEPT QUERIES (V2 API)
    // ========================================================================
    
    /**
     * Find concepts related to given concept
     * - Returns neighbor NodeIDs above threshold
     * - Sorted by edge weight (descending)
     */
    std::vector<NodeID> find_related(NodeID concept_id, float threshold = 0.5f);
    
    /**
     * Multi-hop query from start concept
     * - Returns all reachable concepts within max_hops
     * - Returns pairs of (NodeID, distance)
     */
    std::vector<std::pair<NodeID, int>> multi_hop_query(NodeID start, int max_hops = 5);
    
    /**
     * Find concepts by label substring
     * - Wraps v1's find_nodes
     */
    std::vector<NodeID> find_by_label(const std::string& substring);
    
    /**
     * Get concept label
     */
    std::string get_label(NodeID concept_id);
    
    /**
     * Get edge weight between concepts
     */
    float get_edge_weight(NodeID from, NodeID to);
    
    // ========================================================================
    // CONCEPT CREATION (V2 API - Respects readonly_mode)
    // ========================================================================
    
    /**
     * Create new concept
     * - Adds to v1 graph (if not readonly)
     * - Returns existing if already exists
     */
    NodeID create_concept(const std::string& label, uint8_t type = 0);
    
    /**
     * Add relation between concepts
     * - Creates edge in v1 graph (if not readonly)
     */
    bool add_relation(NodeID from, NodeID to, uint8_t relation_type, float weight = 1.0f);
    
    /**
     * Strengthen existing relation (Hebbian update)
     */
    bool strengthen_relation(NodeID from, NodeID to, float increment = 0.1f);
    
    // ========================================================================
    // MODE CONTROL
    // ========================================================================
    
    /**
     * Set readonly mode (safe during migration)
     */
    void set_readonly_mode(bool readonly);
    
    /**
     * Check if readonly
     */
    bool is_readonly() const { return config_.readonly_mode; }
    
    // ========================================================================
    // STATISTICS & INTROSPECTION
    // ========================================================================
    
    /**
     * Get v1 graph statistics
     */
    struct GraphStats {
        size_t node_count;
        size_t edge_count;
        size_t exact_edges;
        size_t leap_edges;
        size_t storage_bytes;
    };
    
    GraphStats get_graph_stats() const;
    
    /**
     * Bridge statistics
     */
    struct Stats {
        size_t queries_served;
        size_t cache_hits;
        size_t cache_misses;
        size_t concepts_created;
        size_t relations_added;
        size_t readonly_rejections;  // Write attempts in readonly mode
    };
    
    Stats get_stats() const;
    void reset_stats();
    
    // ========================================================================
    // PERSISTENCE (V1 Graph Save/Load)
    // ========================================================================
    
    /**
     * Save v1 graph to disk
     */
    bool save(const std::string& nodes_path, const std::string& edges_path);
    
    /**
     * Load v1 graph from disk
     */
    bool load(const std::string& nodes_path, const std::string& edges_path);
    
    // ========================================================================
    // CONFIGURATION
    // ========================================================================
    
    void set_config(const Config& config);
    Config get_config() const;
    
private:
    melvin::AtomicGraph* v1_graph_;  // Reference to v1 graph (not owned)
    Config config_;
    Stats stats_;
    
    // Query cache (LRU)
    struct CacheEntry {
        NodeID key;
        std::vector<NodeID> neighbors;
        Timestamp cached_at;
    };
    std::vector<CacheEntry> query_cache_;
    
    // Helpers
    std::vector<NodeID> get_neighbors_cached(NodeID node_id);
    void update_cache(NodeID key, const std::vector<NodeID>& neighbors);
    bool check_cache(NodeID key, std::vector<NodeID>& out);
};

// ============================================================================
// HELPER FUNCTIONS
// ============================================================================

/**
 * Convert v1 Node to v2 NodeID (compatible)
 */
inline NodeID v1_to_v2_node_id(uint64_t v1_id) {
    // V1 uses uint64_t, v2 uses uint16_t
    // For compatibility, use lower 16 bits (works if v1 IDs < 65536)
    return static_cast<NodeID>(v1_id & 0xFFFF);
}

/**
 * Convert v2 NodeID to v1 format
 */
inline uint64_t v2_to_v1_node_id(NodeID v2_id) {
    return static_cast<uint64_t>(v2_id);
}

} // namespace melvin::v2::memory

