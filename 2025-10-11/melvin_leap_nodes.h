#pragma once

#include "melvin_types.h"
#include "predictive_sampler.h"
#include <vector>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <memory>
#include <cstdint>

namespace melvin {

// ==================== CONFIGURATION ====================

struct LeapConfig {
    bool enable_leap_nodes = true;           // Master switch for leap system
    float leap_entropy_threshold = 0.6f;     // Trigger leap when entropy > this
    float lambda_leap_bias = 0.3f;           // Strength of leap bias field
    float leap_success_threshold = 0.7f;     // Consider leap successful if coherence gain > this
    int min_cluster_size = 3;                // Minimum nodes in a cluster
    float min_cluster_cohesion = 0.4f;       // Minimum internal similarity
    int max_clusters_per_leap = 3;           // Max clusters to merge into one LeapNode
    int leap_promotion_threshold = 3;        // Successful uses before promotion to permanent
    float novelty_bonus = 0.2f;              // Bias toward under-represented concepts
    int repetition_threshold = 3;            // Consider repetitive if same node appears N times
    int context_window_for_repetition = 5;   // Look back this many tokens for repetition
    float entropy_improvement_threshold = 0.15f; // Min entropy drop to consider leap successful
    float coherence_improvement_threshold = 0.1f; // Min coherence gain to consider leap successful
};

// ==================== SEMANTIC CLUSTER ====================

// A group of semantically related nodes with strong internal connections
struct SemanticCluster {
    std::vector<uint64_t> members;           // Node IDs in this cluster
    float activation;                        // Mean activation across members
    float cohesion;                          // Internal similarity/connectivity
    float novelty;                           // How under-represented in current context
    std::vector<float> centroid;             // Mean embedding of all members
    std::string dominant_concept;            // Most representative node text
    uint64_t last_used_time;                 // For tracking novelty
    
    SemanticCluster() : activation(0.0f), cohesion(0.0f), novelty(1.0f), last_used_time(0) {}
};

// ==================== LEAP NODE ====================

// Temporary or permanent generalized concept formed from cluster merging
struct LeapNode {
    std::vector<uint64_t> members;           // All nodes from merged clusters
    std::vector<int> source_clusters;        // Which clusters were merged
    float activation;                        // Weighted mean of member activations
    float cohesion;                          // Average pairwise similarity
    std::vector<float> concept_vector;       // Centroid embedding
    std::string label;                       // Symbolic name (e.g., "well-being_leap_t42")
    std::vector<int> supporting_tokens;      // Token indices where this leap was triggered
    float success_score;                     // Running success metric
    int usage_count;                         // Times this leap pattern was used
    int success_count;                       // Times it led to coherent output
    int failure_count;                       // Times it led to nonsense
    bool is_permanent;                       // Promoted to permanent node?
    uint64_t node_id;                        // If permanent, its node ID in graph
    uint64_t creation_time;                  // When this leap was created
    
    // Metrics for learning
    float entropy_before;                    // Entropy when leap was triggered
    float entropy_after;                     // Entropy after applying leap
    float coherence_gain;                    // Improvement in output coherence
    
    LeapNode() : activation(0.0f), cohesion(0.0f), success_score(0.0f),
                 usage_count(0), success_count(0), failure_count(0),
                 is_permanent(false), node_id(0), creation_time(0),
                 entropy_before(0.0f), entropy_after(0.0f), coherence_gain(0.0f) {}
};

// ==================== LEAP BIAS FIELD ====================

// Bias values for tokens/nodes to guide predictions
using LeapBiasField = std::unordered_map<uint64_t, float>;

// ==================== CLUSTER MANAGER ====================

// Manages detection and tracking of semantic clusters in the knowledge graph
class ClusterManager {
private:
    std::vector<SemanticCluster> active_clusters_;
    std::unordered_map<uint64_t, int> node_to_cluster_;  // Quick lookup
    LeapConfig config_;
    uint64_t last_update_time_;
    
    // Similarity cache for performance
    std::unordered_map<std::string, float> similarity_cache_;
    
public:
    ClusterManager(const LeapConfig& config = LeapConfig());
    ~ClusterManager() = default;
    
    // Main cluster operations
    void update_clusters(const std::vector<uint64_t>& active_nodes);
    std::vector<SemanticCluster> get_active_clusters() const;
    std::vector<SemanticCluster> get_top_clusters(int n, const std::vector<uint64_t>& context);
    
    // Cluster analysis
    float compute_cluster_cohesion(const std::vector<uint64_t>& members);
    float compute_cluster_novelty(const SemanticCluster& cluster, 
                                   const std::vector<uint64_t>& context);
    std::vector<float> compute_cluster_centroid(const std::vector<uint64_t>& members);
    
    // Utilities
    void clear_stale_clusters(uint64_t max_age_ns);
    int get_cluster_for_node(uint64_t node_id) const;
    
private:
    // Internal methods
    void detect_clusters(const std::vector<uint64_t>& active_nodes);
    std::vector<std::vector<uint64_t>> find_connected_components(
        const std::vector<uint64_t>& nodes, 
        float min_similarity);
    float get_cached_similarity(uint64_t a, uint64_t b);
    std::string make_cache_key(uint64_t a, uint64_t b) const;
};

// ==================== LEAP CONTROLLER ====================

// Main controller for the leap node system
class LeapController {
private:
    ClusterManager cluster_manager_;
    std::vector<LeapNode> active_leaps_;        // Currently active temporary leaps
    std::vector<LeapNode> permanent_leaps_;     // Promoted to permanent nodes
    std::unordered_map<std::string, int> leap_pattern_index_;  // Pattern signature -> index
    LeapConfig config_;
    
    // Statistics
    int total_leaps_created_;
    int total_leaps_succeeded_;
    int total_leaps_failed_;
    int total_promotions_;
    
public:
    LeapController(const LeapConfig& config = LeapConfig());
    ~LeapController() = default;
    
    // Main leap detection and creation
    bool should_trigger_leap(const std::vector<Candidate>& candidates,
                            const std::vector<uint64_t>& context);
    LeapNode create_leap_node(const std::vector<uint64_t>& context,
                             const std::vector<Candidate>& candidates);
    
    // Bias computation
    LeapBiasField compute_leap_bias(const LeapNode& leap_node);
    void apply_leap_bias_to_candidates(std::vector<Candidate>& candidates,
                                       const LeapBiasField& bias_field);
    
    // Learning and feedback
    void record_leap_usage(LeapNode& leap_node, 
                          float entropy_after,
                          float coherence_gain);
    void reinforce_success(LeapNode& leap_node);
    void penalize_failure(LeapNode& leap_node);
    void promote_to_permanent(LeapNode& leap_node);
    
    // Leap lifecycle management
    void cleanup_failed_leaps();
    void update_leap_novelty(const std::vector<uint64_t>& context);
    
    // Query interface
    const std::vector<LeapNode>& get_active_leaps() const { return active_leaps_; }
    const std::vector<LeapNode>& get_permanent_leaps() const { return permanent_leaps_; }
    LeapNode* find_matching_leap_pattern(const std::vector<int>& cluster_ids);
    
    // Statistics
    void print_statistics() const;
    
    // Configuration
    void set_config(const LeapConfig& config) { config_ = config; }
    const LeapConfig& get_config() const { return config_; }
    
private:
    // Internal helpers
    float compute_entropy(const std::vector<Candidate>& candidates);
    int detect_repetition(const std::vector<uint64_t>& context);
    std::string generate_leap_label(const LeapNode& leap_node, int leap_index);
    std::string compute_leap_pattern_signature(const std::vector<int>& cluster_ids);
    float compute_coherence_score(const std::vector<uint64_t>& generated_path);
    std::vector<uint64_t> get_connected_nodes(const LeapNode& leap_node);
};

// ==================== INTEGRATION HELPERS ====================

// Helper to integrate leap system into existing predictive generation
class LeapIntegrator {
public:
    static void apply_leap_to_generation(
        std::vector<Candidate>& candidates,
        const std::vector<uint64_t>& context,
        LeapController& controller,
        bool& leap_triggered);
    
    static void record_generation_feedback(
        LeapController& controller,
        const std::vector<uint64_t>& generated_path,
        float initial_entropy);
};

// ==================== UTILITIES ====================

// Compute entropy of a probability distribution
float compute_entropy(const std::vector<Candidate>& candidates);

// Compute coherence of a path (how well nodes connect semantically)
float compute_path_coherence(const std::vector<uint64_t>& path);

// Get embedding for a node
std::vector<float> get_node_embedding(uint64_t node_id);

// Get text representation of a node
std::string get_node_text(uint64_t node_id);

// Compute similarity between two embeddings (cosine similarity)
float compute_similarity(const std::vector<float>& a, const std::vector<float>& b);

} // namespace melvin


