#pragma once

#include "melvin_types.h"
#include "storage.h"
#include "scoring.hpp"
#include <vector>
#include <queue>
#include <unordered_set>
#include <unordered_map>
#include <random>

namespace melvin {

// Forward declarations
class Store;
struct Query;
struct ScoringContext;

// Beam search parameters
struct BeamParams {
    size_t beam_width = 32;           // Maximum number of paths to keep
    size_t max_depth = 5;             // Maximum path length
    size_t top_k = 8;                 // Top-k expansion at each step
    double top_p = 0.9;               // Top-p sampling threshold
    double stop_threshold = 0.05;     // Stop when marginal gain < threshold
    bool enable_loop_detection = true; // Detect and penalize loops
    bool enable_stochastic_expansion = true; // Use top-p sampling
    
    BeamParams() = default;
};

// Path structure for beam search
struct BeamPath {
    std::vector<EdgeRec> edges;
    std::vector<NodeID> nodes;        // Nodes visited in order
    double score;
    double confidence;
    bool is_complete;
    uint64_t timestamp;
    
    // Loop detection
    std::unordered_set<NodeID, NodeIDHash> visited_nodes;
    std::unordered_set<EdgeID, EdgeIDHash> visited_edges;
    
    BeamPath() : score(0.0), confidence(0.0), is_complete(false), timestamp(0) {}
    
    // Copy constructor
    BeamPath(const BeamPath& other) = default;
    BeamPath& operator=(const BeamPath& other) = default;
    
    // Move constructor
    BeamPath(BeamPath&& other) = default;
    BeamPath& operator=(BeamPath&& other) = default;
    
    // Comparison for priority queue (max-heap)
    bool operator<(const BeamPath& other) const {
        return score < other.score;
    }
    
    // Add edge to path
    void add_edge(const EdgeRec& edge, const NodeID& target_node, double edge_score);
    
    // Check if path contains a loop
    bool has_loop() const;
    
    // Get current node (last node in path)
    NodeID get_current_node() const;
    
    // Get path length
    size_t get_length() const { return edges.size(); }
    
    // Check if path is at max depth
    bool is_at_max_depth(size_t max_depth) const;
};

// Beam search state
class BeamSearchState {
private:
    std::priority_queue<BeamPath> beam_;
    std::vector<BeamPath> completed_paths_;
    std::unordered_set<std::string> path_signatures_; // For deduplication
    
    BeamParams params_;
    size_t current_depth_;
    
public:
    BeamSearchState(const BeamParams& params) : params_(params), current_depth_(0) {}
    
    // Add path to beam
    void add_path(const BeamPath& path);
    
    // Get best paths from beam
    std::vector<BeamPath> get_best_paths(size_t k) const;
    
    // Get all completed paths
    const std::vector<BeamPath>& get_completed_paths() const { return completed_paths_; }
    
    // Expand beam to next depth
    void expand_beam(const std::vector<BeamPath>& new_paths);
    
    // Check if beam is empty
    bool is_empty() const { return beam_.empty(); }
    
    // Get beam size
    size_t size() const { return beam_.size(); }
    
    // Clear beam
    void clear() { 
        while (!beam_.empty()) beam_.pop();
        completed_paths_.clear();
        path_signatures_.clear();
        current_depth_ = 0;
    }
    
    // Get current depth
    size_t get_current_depth() const { return current_depth_; }
    
private:
    // Generate path signature for deduplication
    std::string generate_path_signature(const BeamPath& path) const;
};

// Stochastic expansion utilities
class StochasticExpander {
private:
    mutable std::mt19937 rng_;
    
public:
    StochasticExpander(uint64_t seed = 0) : rng_(seed) {}
    
    // Select edges using top-p sampling
    std::vector<EdgeRec> select_edges_top_p(const std::vector<std::pair<EdgeRec, double>>& scored_edges, 
                                           double top_p) const;
    
    // Select edges using top-k
    std::vector<EdgeRec> select_edges_top_k(const std::vector<std::pair<EdgeRec, double>>& scored_edges, 
                                           size_t top_k) const;
    
    // Compute cumulative probabilities for sampling
    std::vector<double> compute_cumulative_probs(const std::vector<double>& scores) const;
    
    // Sample from categorical distribution
    size_t sample_categorical(const std::vector<double>& probs) const;
    
    void set_seed(uint64_t seed) { rng_.seed(seed); }
};

// Main beam search engine
class BeamSearchEngine {
private:
    BeamParams params_;
    GraphScorer scorer_;
    StochasticExpander expander_;
    
public:
    BeamSearchEngine(const BeamParams& params = BeamParams(), 
                    const ScoringWeights& weights = ScoringWeights())
        : params_(params), scorer_(weights) {}
    
    // Main beam search interface
    std::vector<BeamPath> search(const Query& query, 
                                const NodeID& start_node,
                                Store* store);
    
    // Search with custom parameters
    std::vector<BeamPath> search_with_params(const Query& query,
                                            const NodeID& start_node,
                                            Store* store,
                                            const BeamParams& custom_params);
    
    // Single step expansion
    std::vector<BeamPath> expand_path(const BeamPath& path,
                                     const ScoringContext& context,
                                     Store* store);
    
    // Configuration
    void set_params(const BeamParams& params) { params_ = params; }
    void set_scoring_weights(const ScoringWeights& weights) { scorer_.set_weights(weights); }
    void set_random_seed(uint64_t seed) { expander_.set_seed(seed); }
    
    const BeamParams& get_params() const { return params_; }
    const ScoringWeights& get_scoring_weights() const { return scorer_.get_weights(); }
    
private:
    // Helper methods
    bool should_stop_search(const BeamSearchState& state, double best_score) const;
    std::vector<std::pair<EdgeRec, double>> score_outgoing_edges(const BeamPath& path,
                                                               const ScoringContext& context,
                                                               Store* store) const;
    BeamPath create_new_path(const BeamPath& parent_path, 
                           const EdgeRec& edge,
                           double edge_score) const;
    bool is_valid_expansion(const BeamPath& path, const EdgeRec& edge) const;
};

// Multi-start beam search (for queries that might have multiple good starting points)
class MultiStartBeamSearch {
private:
    BeamSearchEngine engine_;
    BeamParams params_;
    
public:
    MultiStartBeamSearch(const BeamParams& params = BeamParams(),
                        const ScoringWeights& weights = ScoringWeights())
        : engine_(params, weights), params_(params) {}
    
    // Search from multiple starting nodes
    std::vector<BeamPath> search_multiple_starts(const Query& query,
                                                const std::vector<NodeID>& start_nodes,
                                                Store* store);
    
    // Find best starting nodes for query
    std::vector<NodeID> find_best_start_nodes(const Query& query,
                                            Store* store,
                                            size_t num_starts = 3) const;
    
    // Merge results from multiple searches
    std::vector<BeamPath> merge_search_results(const std::vector<std::vector<BeamPath>>& results,
                                             size_t max_results = 10) const;
    
    // Configuration
    void set_params(const BeamParams& params) { 
        params_ = params; 
        engine_.set_params(params); 
    }
    
    void set_scoring_weights(const ScoringWeights& weights) { 
        engine_.set_scoring_weights(weights); 
    }
};

// Beam search with caching
class CachedBeamSearch {
private:
    BeamSearchEngine engine_;
    std::unordered_map<std::string, std::vector<BeamPath>> cache_;
    size_t max_cache_size_;
    
public:
    CachedBeamSearch(const BeamParams& params = BeamParams(),
                    const ScoringWeights& weights = ScoringWeights(),
                    size_t max_cache_size = 1000)
        : engine_(params, weights), max_cache_size_(max_cache_size) {}
    
    // Search with caching
    std::vector<BeamPath> search_cached(const Query& query,
                                       const NodeID& start_node,
                                       Store* store);
    
    // Clear cache
    void clear_cache() { cache_.clear(); }
    
    // Get cache statistics
    size_t get_cache_size() const { return cache_.size(); }
    size_t get_cache_hits() const;
    size_t get_cache_misses() const;
    
private:
    // Generate cache key
    std::string generate_cache_key(const Query& query, const NodeID& start_node) const;
    
    // Cache management
    void evict_old_entries();
};

// Utility functions
namespace BeamSearchUtils {
    // Compute path diversity
    double compute_path_diversity(const std::vector<BeamPath>& paths);
    
    // Filter paths by confidence
    std::vector<BeamPath> filter_by_confidence(const std::vector<BeamPath>& paths, 
                                              double min_confidence);
    
    // Sort paths by score
    void sort_by_score(std::vector<BeamPath>& paths);
    
    // Compute path signature
    std::string compute_path_signature(const BeamPath& path);
    
    // Check if two paths are similar
    bool paths_are_similar(const BeamPath& path1, const BeamPath& path2, double threshold = 0.8);
    
    // Merge similar paths
    std::vector<BeamPath> deduplicate_paths(const std::vector<BeamPath>& paths, 
                                          double similarity_threshold = 0.8);
}

} // namespace melvin
