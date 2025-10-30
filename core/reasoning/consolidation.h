#ifndef CONSOLIDATION_H
#define CONSOLIDATION_H

#include <unordered_map>
#include <vector>
#include <deque>

namespace melvin {
namespace reasoning {

// GAP 6: Enhanced Consolidation
struct Experience {
    std::vector<int> activated_nodes;
    std::vector<std::pair<int, int>> active_edges;  // (src, dst) pairs
    float importance;        // How valuable was this experience?
    float timestamp;
    float outcome_reward;    // Success/failure signal
};

struct NodeCluster {
    std::vector<int> member_nodes;
    std::vector<float> centroid_embedding;
    int frequency;
    float coherence;
    int abstract_node_id;
};

class Consolidator {
public:
    Consolidator();
    
    // GAP 6: Enhanced consolidation methods
    void consolidate_full(
        std::unordered_map<int, std::vector<std::pair<int, float>>>& graph,
        std::unordered_map<int, std::vector<float>>& embeddings,
        const std::unordered_map<int, float>& activation_history,
        const std::deque<Experience>& experiences,
        float meta_learning_rate
    );
    
    // Experience replay (strengthen important patterns)
    void replay_experiences(
        std::unordered_map<int, std::vector<std::pair<int, float>>>& graph,
        const std::deque<Experience>& experiences,
        int num_replays = 10
    );
    
    // Edge pruning (remove weak/unused connections)
    int prune_weak_edges(
        std::unordered_map<int, std::vector<std::pair<int, float>>>& graph,
        float current_time
    );
    
    // Abstraction formation (create higher-level concepts)
    std::vector<NodeCluster> form_abstractions(
        const std::unordered_map<int, std::vector<std::pair<int, float>>>& graph,
        const std::unordered_map<int, std::vector<float>>& embeddings,
        int min_frequency = 100
    );
    
    // Node merging (combine similar nodes)
    int merge_similar_nodes(
        std::unordered_map<int, std::vector<std::pair<int, float>>>& graph,
        std::unordered_map<int, std::vector<float>>& embeddings
    );
    
    // Original interface (enhanced)
    void consolidate(
        std::unordered_map<int, std::vector<std::pair<int, float>>>& graph,
        const std::unordered_map<int, float>& activation_history,
        const std::vector<std::vector<int>>& episodes
    );
    
    bool should_trigger(int reasoning_session_count, float idle_time);
    
    // Statistics
    struct Stats {
        int experiences_replayed = 0;
        int edges_pruned = 0;
        int abstractions_formed = 0;
        int nodes_merged = 0;
    };
    
    const Stats& get_stats() const { return stats_; }
    void reset_stats() { stats_ = Stats(); }
    
private:
    float strengthening_rate_;
    float pruning_threshold_;
    float merge_threshold_;
    
    // Enhanced parameters
    float replay_strength_ = 0.05f;
    float edge_age_threshold_ = 1000000.0f;  // Time units
    int min_activation_count_ = 3;
    
    Stats stats_;
    
    // Helper methods
    float compute_similarity(const std::vector<float>& a, const std::vector<float>& b);
    std::vector<int> find_frequent_pattern(
        const std::unordered_map<int, std::vector<std::pair<int, float>>>& graph,
        int seed_node,
        int min_frequency
    );
};

} // namespace reasoning
} // namespace melvin

#endif // CONSOLIDATION_H
