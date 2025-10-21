#pragma once

#include "types.h"
#include "storage.h"
#include <vector>
#include <memory>
#include <chrono>

namespace melvin {
namespace episodic {

/**
 * 📚 Episodic Memory - Temporal Sequence Tracking
 * 
 * Tracks experiences as temporal sequences of EXACT node chains.
 * Enables time-based reasoning and autobiographical memory.
 * 
 * Key Features:
 * - Episodes group related experiences with temporal ordering
 * - Query by time range or semantic similarity
 * - Consolidation merges similar episodes
 * - Enables "what happened when" queries
 */
class EpisodicMemory {
public:
    struct Config {
        size_t max_episodes = 10000;          // Maximum episodes to keep
        uint64_t consolidation_interval_ms = 3600000; // 1 hour
        float similarity_threshold = 0.7f;     // For episode merging
        bool auto_consolidate = true;          // Automatic consolidation
        bool verbose = false;                  // Logging
        
        Config();
    };
    
    explicit EpisodicMemory(Storage* storage = nullptr, const Config& config = Config());
    ~EpisodicMemory();
    
    // ========================================================================
    // EPISODE MANAGEMENT
    // ========================================================================
    
    /**
     * Create a new episode with context
     * Returns episode ID
     */
    uint32_t create_episode(const std::string& context);
    
    /**
     * End the current active episode
     */
    void end_episode(uint32_t episode_id);
    
    /**
     * Add a node to current episode
     */
    void add_node(uint32_t episode_id, NodeID node_id);
    
    /**
     * Add an edge to current episode
     */
    void add_edge(uint32_t episode_id, EdgeID edge_id);
    
    /**
     * Get episode by ID
     */
    bool get_episode(uint32_t episode_id, Episode& out) const;
    
    /**
     * Get all episodes
     */
    std::vector<Episode> get_all_episodes() const;
    
    // ========================================================================
    // TEMPORAL QUERIES
    // ========================================================================
    
    /**
     * Recall episodes within a time range
     */
    std::vector<Episode> recall_timerange(uint64_t start_time, uint64_t end_time) const;
    
    /**
     * Recall episodes similar to current context
     * Uses node overlap to measure similarity
     */
    std::vector<Episode> recall_similar(const std::vector<NodeID>& context, size_t top_k = 5) const;
    
    /**
     * Recall recent episodes (last N)
     */
    std::vector<Episode> recall_recent(size_t count = 10) const;
    
    /**
     * Reconstruct reasoning path for an episode
     * Returns the sequence of nodes and edges in temporal order
     */
    ReasoningPath reconstruct_episode(uint32_t episode_id) const;
    
    // ========================================================================
    // CONSOLIDATION
    // ========================================================================
    
    /**
     * Consolidate episodes: merge similar ones, compress old ones
     */
    void consolidate();
    
    /**
     * Prune old episodes beyond max_episodes limit
     */
    void prune_old_episodes();
    
    // ========================================================================
    // STATISTICS
    // ========================================================================
    
    struct Stats {
        size_t total_episodes = 0;
        size_t active_episodes = 0;
        uint64_t oldest_episode_time = 0;
        uint64_t newest_episode_time = 0;
        float avg_episode_size = 0.0f;
        size_t total_nodes_in_episodes = 0;
        size_t consolidations_performed = 0;
    };
    
    Stats get_stats() const;
    void print_stats() const;
    
    // ========================================================================
    // PERSISTENCE
    // ========================================================================
    
    /**
     * Save episodes to disk
     */
    bool save(const std::string& path) const;
    
    /**
     * Load episodes from disk
     */
    bool load(const std::string& path);
    
    /**
     * Set storage backend for node/edge lookups
     */
    void set_storage(Storage* storage);
    
private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

/**
 * Helper: Get current timestamp in milliseconds since epoch
 */
inline uint64_t get_timestamp_ms() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();
}

} // namespace episodic
} // namespace melvin

