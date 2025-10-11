#pragma once

#include <vector>
#include <string>
#include <unordered_map>
#include <cstdint>

// Forward declarations
struct Node;
struct Edge;

namespace melvin {
namespace reasoning {

// Multi-hop search configuration
struct MultihopConfig {
    int max_hops = 3;           // Maximum reasoning depth
    int beam_width = 5;         // Beam search width
    float length_penalty = 0.9f; // Prefer shorter paths
    
    // Relation type priors (weight multipliers)
    std::unordered_map<std::string, float> relation_priors = {
        {"ARE", 1.0f},
        {"IS_A", 1.0f},
        {"DRINK", 0.9f},
        {"NEED", 0.9f},
        {"HAVE", 0.8f},
        {"PART_OF", 0.8f},
        {"PRODUCES", 0.7f}
    };
};

// Single hop in a reasoning path
struct ReasoningHop {
    uint64_t from_node;
    uint64_t to_node;
    uint64_t edge_id;
    std::string relation;
    float weight;
};

// Complete reasoning path from query to answer
struct ReasoningPath {
    std::vector<ReasoningHop> hops;
    float total_score;
    std::string answer;         // Final node text
    
    int hop_count() const { return static_cast<int>(hops.size()); }
    
    // Get human-readable path string
    std::string to_string(const std::unordered_map<uint64_t, Node>& nodes) const;
};

// Result of multi-hop search
struct MultihopResult {
    bool success;
    std::string answer;
    ReasoningPath best_path;
    std::vector<ReasoningPath> all_paths;  // Top-k paths
    int nodes_explored;
    int paths_generated;
    std::string error_message;
};

// Multi-hop reasoning engine
class MultihopEngine {
public:
    explicit MultihopEngine(const MultihopConfig& config = MultihopConfig());
    
    // Search for answer to query using multi-hop reasoning
    MultihopResult search(
        const std::string& query,
        const std::unordered_map<uint64_t, Node>& nodes,
        const std::vector<Edge>& edges,
        const std::unordered_map<uint64_t, std::vector<size_t>>& adjacency
    );
    
    // Find start nodes relevant to query
    std::vector<uint64_t> find_start_nodes(
        const std::string& query,
        const std::unordered_map<uint64_t, Node>& nodes
    );
    
    // Get configuration
    const MultihopConfig& config() const { return config_; }
    
    // Update configuration
    void set_config(const MultihopConfig& config) { config_ = config; }
    
private:
    MultihopConfig config_;
    
    // Beam search implementation
    std::vector<ReasoningPath> beam_search(
        const std::vector<uint64_t>& start_nodes,
        const std::unordered_map<uint64_t, Node>& nodes,
        const std::vector<Edge>& edges,
        const std::unordered_map<uint64_t, std::vector<size_t>>& adjacency,
        int max_hops
    );
    
    // Expand a single path by one hop
    std::vector<ReasoningPath> expand_path(
        const ReasoningPath& path,
        const std::unordered_map<uint64_t, Node>& nodes,
        const std::vector<Edge>& edges,
        const std::unordered_map<uint64_t, std::vector<size_t>>& adjacency
    );
    
    // Score a reasoning path
    float score_path(const ReasoningPath& path) const;
    
    // Get relation prior weight
    float get_relation_prior(const std::string& relation) const;
    
    // Check if node matches query intent
    bool node_matches_query(
        uint64_t node_id,
        const std::string& query,
        const std::unordered_map<uint64_t, Node>& nodes
    );
};

// Utility functions
std::string format_path(
    const ReasoningPath& path,
    const std::unordered_map<uint64_t, Node>& nodes
);

} // namespace reasoning
} // namespace melvin

